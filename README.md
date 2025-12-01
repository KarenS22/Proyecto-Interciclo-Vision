# Proyecto Interciclo

Resumen
-------
Proyecto para procesamiento y segmentación de cortes CT (DICOM). Incluye:
- Lectura de series DICOM usando ITK.
- Preprocesamiento y denoising (OpenCV + llamada a servicio Flask/DnCNN).
- Interfaz interactiva con ventanas OpenCV para selección de slice, calibración y visualización de resultados.

Cambios realizados
------------------
- Las ventanas que muestran imágenes ahora usan `WINDOW_NORMAL` y se les aplica `resizeWindow(...)` para que las imágenes y ventanas sean más grandes y legibles.
- Se aumentaron factores de escalado en comparaciones (más grandes) y el menú de selección se hizo más amplio.

Requisitos
----------
- Sistema: Linux (probado en Ubuntu-ish)
- Compilación C++:
  - CMake >= 3.10
  - Compilador C++ (g++/clang++)
  - OpenCV (>=3.4 / 4.x)
  - ITK (para lectura DICOM)
- Python (opcional): para el servicio Flask que aplica DnCNN
  - Requerimientos en `server.py` / `client.py` (ver comentarios en esos archivos)

Cómo compilar
------------
Desde la raíz del proyecto (shell zsh):

```bash
# Si ya existe carpeta build (este repo ya tiene una), usarla:
cmake --build build -- -j$(nproc)

# Si no existe o deseas una build limpia:
# mkdir -p build && cd build
# cmake ..
# make -j$(nproc)
```

Cómo ejecutar
-------------
Ejemplo de ejecución (ruta a carpeta con archivos DICOM):

```bash
./ct_processor /ruta/a/serie_dicom 195,200
```

Notas importantes
-----------------
- El proyecto abre ventanas OpenCV que ahora son redimensionables y por defecto se ajustan a tamaños más grandes (por ejemplo 1200x700 o hasta 1600x900 en comparaciones). Si tu pantalla es pequeña ajusta estos valores en `Interfaz.cpp`.
- Para la parte de DnCNN se llama a un servicio Flask (archivo `server.py`). Asegúrate de tenerlo corriendo si quieres usar la ruta IA. Puedes probar sin servidor; el código guarda imágenes intermedias en `output/...`.

Pruebas rápidas
---------------
- Ejecuta el programa con una serie DICOM pequeña y verifica que las ventanas de "Calibrando Tejidos", "Visualizacion Color", y las comparaciones salgan más grandes.

Siguientes pasos sugeridos
-------------------------
- Añadir un flag/config para controlar el tamaño máximo de ventana desde línea de comandos.
- Implementar ajustes automáticos para adaptarse a la resolución de pantalla del usuario.

Contacto
--------
Si quieres que ajuste los tamaños por defecto (más grandes o más pequeños), dime qué dimensiones prefieres y lo cambio.
