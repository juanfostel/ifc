#define _GNU_SOURCE

#include "ftrapi_compat.h"

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define TEMPLATE_DIR "/var/lib/futronic-fs81"
#define DEFAULT_FAR 107374182U
#define DEFAULT_MODELS 3U

static const char *program_name = "futronic-auth";

static void usage(FILE *out) {
    fprintf(out,
            "Uso:\n"
            "  %s enroll [usuario]\n"
            "  %s verify [usuario]\n"
            "  %s verify --pam-user\n\n"
            "Si se usa --pam-user, toma el usuario desde PAM_USER.\n",
            program_name, program_name, program_name);
}

static const char *retcode_name(FTRAPI_RESULT code) {
    switch (code) {
    case FTR_RETCODE_OK: return "OK";
    case FTR_RETCODE_NO_MEMORY: return "sin memoria";
    case FTR_RETCODE_INVALID_ARG: return "argumento invalido";
    case FTR_RETCODE_ALREADY_IN_USE: return "API ya inicializada";
    case FTR_RETCODE_INVALID_PURPOSE: return "proposito invalido";
    case FTR_RETCODE_INTERNAL_ERROR: return "error interno";
    case FTR_RETCODE_UNABLE_TO_CAPTURE: return "no se pudo capturar";
    case FTR_RETCODE_CANCELED_BY_USER: return "cancelado";
    case FTR_RETCODE_NO_MORE_RETRIES: return "sin reintentos";
    case FTR_RETCODE_FRAME_SOURCE_NOT_SET: return "lector no seleccionado";
    case FTR_RETCODE_DEVICE_NOT_CONNECTED: return "lector no conectado";
    case FTR_RETCODE_DEVICE_FAILURE: return "fallo del lector";
    case FTR_RETCODE_EMPTY_FRAME: return "captura vacia";
    case FTR_RETCODE_FAKE_SOURCE: return "dedo falso detectado";
    default: return "error desconocido";
    }
}

static void control_callback(FTR_USER_CTX context, FTR_STATE state_mask, FTR_RESPONSE *response,
                             FTR_SIGNAL signal, FTR_BITMAP *bitmap) {
    (void)context;
    (void)bitmap;

    if ((state_mask & FTR_STATE_SIGNAL_PROVIDED) && response) {
        *response = FTR_CONTINUE;
    }

    if (signal == FTR_SIGNAL_TOUCH_SENSOR) {
        fprintf(stderr, "Toque el lector Futronic...\n");
    } else if (signal == FTR_SIGNAL_TAKE_OFF) {
        fprintf(stderr, "Retire el dedo...\n");
    } else if (signal == FTR_SIGNAL_FAKE_SOURCE) {
        fprintf(stderr, "El lector reporto posible dedo falso.\n");
    }
}

static int mkdir_private(const char *path) {
    if (mkdir(path, 0700) == -1 && errno != EEXIST) {
        perror("mkdir");
        return -1;
    }
    if (chmod(path, 0700) == -1) {
        perror("chmod");
        return -1;
    }
    return 0;
}

static int validate_user(const char *user) {
    if (!user || !*user) {
        fprintf(stderr, "Usuario vacio.\n");
        return -1;
    }

    for (const char *p = user; *p; ++p) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-' || *p == '.')) {
            fprintf(stderr, "Usuario no permitido: %s\n", user);
            return -1;
        }
    }

    errno = 0;
    if (!getpwnam(user)) {
        fprintf(stderr, "El usuario no existe en el sistema: %s\n", user);
        return -1;
    }
    return 0;
}

static int validate_setuid_target(const char *user) {
    uid_t real_uid = getuid();
    if (real_uid == 0) {
        return 0;
    }

    struct passwd *pw = getpwuid(real_uid);
    if (!pw || strcmp(pw->pw_name, user) != 0) {
        fprintf(stderr, "Usuario PAM no coincide con el usuario real.\n");
        return -1;
    }
    return 0;
}

static int template_path(const char *user, char *buf, size_t size) {
    int written = snprintf(buf, size, "%s/%s.tpl", TEMPLATE_DIR, user);
    if (written < 0 || (size_t)written >= size) {
        fprintf(stderr, "Ruta de plantilla demasiado larga.\n");
        return -1;
    }
    return 0;
}

static int write_all(int fd, const void *buf, size_t count) {
    const uint8_t *p = (const uint8_t *)buf;
    while (count > 0) {
        ssize_t written = write(fd, p, count);
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (written == 0) {
            return -1;
        }
        p += written;
        count -= (size_t)written;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t count) {
    uint8_t *p = (uint8_t *)buf;
    while (count > 0) {
        ssize_t n = read(fd, p, count);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        p += n;
        count -= (size_t)n;
    }
    return 0;
}

static int sdk_init(void) {
    FTRAPI_RESULT rc = FTRInitialize();
    if (rc != FTR_RETCODE_OK) {
        fprintf(stderr, "FTRInitialize fallo: %s (%d)\n", retcode_name(rc), rc);
        return -1;
    }

    rc = FTRSetParam(FTR_PARAM_CB_FRAME_SOURCE, FSD_FUTRONIC_USB);
    if (rc != FTR_RETCODE_OK) {
        fprintf(stderr, "No se pudo seleccionar el lector USB: %s (%d)\n", retcode_name(rc), rc);
        FTRTerminate();
        return -1;
    }

    FTRSetParam(FTR_PARAM_MAX_MODELS, DEFAULT_MODELS);
    FTRSetParam(FTR_PARAM_CB_CONTROL, (uintptr_t)control_callback);
    FTRSetParam(FTR_PARAM_MAX_FAR_REQUESTED, DEFAULT_FAR);
    return 0;
}

static int get_template_size(FTR_DWORD *size) {
    FTRAPI_RESULT rc = FTRGetParam(FTR_PARAM_MAX_TEMPLATE_SIZE, size);
    if (rc != FTR_RETCODE_OK || *size == 0 || *size > 1024U * 1024U) {
        fprintf(stderr, "No se pudo obtener el tamano de plantilla: %s (%d)\n", retcode_name(rc), rc);
        return -1;
    }
    return 0;
}

static int write_template_file(const char *path, const FTR_DATA *tpl) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    uint8_t header[9] = {'F', 'S', '8', '1', 1, 0, 0, 0, 0};
    header[5] = (uint8_t)(tpl->dwSize & 0xff);
    header[6] = (uint8_t)((tpl->dwSize >> 8) & 0xff);
    header[7] = (uint8_t)((tpl->dwSize >> 16) & 0xff);
    header[8] = (uint8_t)((tpl->dwSize >> 24) & 0xff);

    if (write_all(fd, header, sizeof(header)) == -1 ||
        write_all(fd, tpl->pData, tpl->dwSize) == -1) {
        perror("write");
        close(fd);
        return -1;
    }

    if (fchmod(fd, 0600) == -1) {
        perror("fchmod");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int read_template_file(const char *path, FTR_DATA *tpl) {
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd == -1) {
        return -1;
    }

    uint8_t header[9];
    if (read_all(fd, header, sizeof(header)) == -1 || memcmp(header, "FS81", 4) != 0) {
        close(fd);
        return -1;
    }

    FTR_DWORD size = (FTR_DWORD)header[5] | ((FTR_DWORD)header[6] << 8) |
                      ((FTR_DWORD)header[7] << 16) | ((FTR_DWORD)header[8] << 24);
    if (size == 0 || size > 1024U * 1024U) {
        close(fd);
        return -1;
    }

    void *data = calloc(1, size);
    if (!data) {
        close(fd);
        return -1;
    }

    if (read_all(fd, data, size) == -1) {
        free(data);
        close(fd);
        return -1;
    }

    close(fd);
    tpl->dwSize = size;
    tpl->pData = data;
    return 0;
}

static int enroll_user(const char *user) {
    if (getuid() != 0) {
        fprintf(stderr, "El enrolamiento debe ejecutarse como root.\n");
        return 1;
    }

    if (validate_user(user) == -1 || mkdir_private(TEMPLATE_DIR) == -1 || sdk_init() == -1) {
        return 1;
    }

    FTR_DWORD max_size = 0;
    if (get_template_size(&max_size) == -1) {
        FTRTerminate();
        return 1;
    }

    FTR_DATA tpl = {.dwSize = max_size, .pData = calloc(1, max_size)};
    if (!tpl.pData) {
        FTRTerminate();
        return 1;
    }

    fprintf(stderr, "Enrolando huella para %s. Siga las indicaciones del lector.\n", user);
    FTR_ENROLL_DATA enroll_data = {.dwSize = sizeof(enroll_data), .dwQuality = 0};
    FTRAPI_RESULT rc = FTREnrollX(NULL, FTR_PURPOSE_ENROLL, &tpl, &enroll_data);
    if (rc != FTR_RETCODE_OK) {
        fprintf(stderr, "FTREnroll fallo: %s (%d)\n", retcode_name(rc), rc);
        free(tpl.pData);
        FTRTerminate();
        return 1;
    }

    char path[512];
    int ok = template_path(user, path, sizeof(path)) == 0 && write_template_file(path, &tpl) == 0;
    free(tpl.pData);
    FTRTerminate();

    if (!ok) {
        return 1;
    }

    fprintf(stderr, "Huella guardada en %s\n", path);
    return 0;
}

static int verify_user(const char *user) {
    if (validate_user(user) == -1 || validate_setuid_target(user) == -1 || sdk_init() == -1) {
        return 1;
    }

    char path[512];
    FTR_DATA tpl = {0};
    if (template_path(user, path, sizeof(path)) == -1 || read_template_file(path, &tpl) == -1) {
        FTRTerminate();
        return 1;
    }

    FTR_BOOL matched = 0;
    FTR_DWORD far = 0;
    FTRAPI_RESULT rc = FTRVerify(NULL, &tpl, &matched, &far);
    free(tpl.pData);
    FTRTerminate();

    if (rc != FTR_RETCODE_OK) {
        fprintf(stderr, "FTRVerify fallo: %s (%d)\n", retcode_name(rc), rc);
        return 1;
    }

    if (!matched) {
        fprintf(stderr, "Huella no coincide para %s.\n", user);
        return 1;
    }

    fprintf(stderr, "Huella verificada para %s.\n", user);
    return 0;
}

int main(int argc, char **argv) {
    program_name = argv[0];
    if (argc < 2) {
        usage(stderr);
        return 2;
    }

    const char *command = argv[1];
    const char *user = NULL;
    if (argc >= 3 && strcmp(argv[2], "--pam-user") == 0) {
        user = getenv("PAM_USER");
    } else if (argc >= 3) {
        user = argv[2];
    } else {
        struct passwd *pw = getpwuid(getuid());
        user = pw ? pw->pw_name : NULL;
    }

    if (strcmp(command, "enroll") == 0) {
        return enroll_user(user);
    }
    if (strcmp(command, "verify") == 0) {
        return verify_user(user);
    }

    usage(stderr);
    return 2;
}
