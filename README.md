 (cd "$(git rev-parse --show-toplevel)" && git apply --3way <<'EOF' 
diff --git a//dev/null b/README.md
index 0000000000000000000000000000000000000000..80036802ee1a010411b8724a361bc849cf89db59 100644
--- a//dev/null
+++ b/README.md
@@ -0,0 +1,33 @@
+# Exportar resultados de Karamba3D a IFC
+
+Este repositorio incluye una definición de Grasshopper (`grasshopper/ExportKarambaToIFC.ghx`) pensada como plantilla para
+obtener el modelo estructural generado por **Karamba3D** y almacenarlo en un archivo IFC sencillo.
+
+## ¿Qué contiene la definición?
+
+* Un componente `GhPython` listo para conectarse a la salida `Model` del componente `Assemble` de Karamba3D.
+* Entradas para especificar la ruta de guardado y activar la exportación.
+* Código en Python que recorre los elementos del modelo (centrado en vigas), genera entidades IFC básicas y escribe un archivo con cabecera IFC4.
+
+## Uso básico
+
+1. Copia el archivo `ExportKarambaToIFC.ghx` a tu carpeta de definiciones o impórtalo en Grasshopper.
+2. Abre la definición y conecta la salida `Model` del componente `Assemble` de Karamba3D al nodo "Export IFC".
+3. Proporciona una ruta completa (por ejemplo `C:\\proyecto\\estructura.ifc`). Si omites la extensión `.ifc`, el script la añadirá automáticamente.
+4. Activa el interruptor de la entrada **G** (`True`) para crear el archivo IFC. El componente devolverá la ruta final escrita y mensajes de log.
+
+## Dependencias
+
+* Rhino + Grasshopper con Karamba3D instalado.
+* El componente `GhPython` requiere acceso a las librerías `KarambaCommon` y `Karamba` que se incluyen con la instalación de Karamba3D.
+
+## Limitaciones y mejoras posibles
+
+* El script produce un IFC con jerarquía mínima (Proyecto > Sitio > Edificio) y elementos de tipo `IfcBeam` representados como polilíneas. No se exportan secciones, materiales ni cargas.
+* Solo se procesan elementos que expongan una propiedad `curve`. Si tienes barras de otro tipo o elementos de placas, será necesario ampliar la lógica.
+* Se generan relaciones `IfcRelContainedInSpatialStructure` individuales por cada elemento. Podría optimizarse agrupándolas.
+* La geometría se serializa como líneas 3D; si se requieren sólidos, deberás complementar la definición con perfiles o extrusiones.
+
+## Registro de cambios
+
+* **v0.1** – Plantilla inicial con exportación de vigas y generación automática de directorios de salida.
 
EOF
)
