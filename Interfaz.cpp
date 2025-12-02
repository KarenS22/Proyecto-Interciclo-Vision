#include "Interfaz.hpp"
#include "Operaciones.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace cv;
using namespace std;

void esperarTeclaS(const string& mensaje) {
    cout << "\n" << mensaje << endl;
    cout << "Presiona 'S' o 's' para continuar..." << endl;
    
    int key = 0;
    while(key != 's' && key != 'S') {
        key = waitKey(0) & 0xFF;
        if(key == 27) { // ESC para salir
            cout << "Proceso cancelado por el usuario." << endl;
            exit(0);
        }
    }
    cout << "Continuando..." << endl;
}

int seleccionarSlice(InputImageType::Pointer image3D, int minSlice, int maxSlice) {
    int sliceActual = minSlice;

    const string windowName = "Seleccionar Slice (Trackbar)";
    namedWindow(windowName, WINDOW_NORMAL);

    cout << "\n========================================" << endl;
    cout << "SELECCIÓN DE SLICE (Trackbar)" << endl;
    cout << "========================================" << endl;
    cout << "Controles:" << endl;
    cout << "  - Mover el trackbar: Navegar entre slices" << endl;
    cout << "  - [S] : Seleccionar slice actual" << endl;
    cout << "  - [ESC] : Salir" << endl;
    cout << "Rango disponible: " << minSlice << " - " << maxSlice << endl;

    // Trackbar callback variable
    int trackPos = minSlice;

    // Create trackbar
    createTrackbar("Slice", windowName, &trackPos, maxSlice);

    while (true) {

        sliceActual = trackPos;  

        Mat slice = itkSliceToMat(image3D, sliceActual);
        Mat display;

        if (slice.cols > 1200) {
            double scale = 1200.0 / slice.cols;
            resize(slice, display, Size(), scale, scale);
        } else {
            display = slice.clone();
        }

        // Convertir a color para mostrar texto
        Mat colorDisplay;
        cvtColor(display, colorDisplay, COLOR_GRAY2BGR);

        // Instrucciones
        putText(colorDisplay, "S: Seleccionar",
                Point(10, colorDisplay.rows - 20),
                FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);

        imshow(windowName, colorDisplay);

        // Ajuste dinámico del tamaño
        int winW = std::min(1400, colorDisplay.cols);
        int winH = std::min(900, colorDisplay.rows);
        resizeWindow(windowName, winW, winH);

        int key = waitKey(30) & 0xFF; // ahora no bloquea

        if (key == 's' || key == 'S') {
            destroyWindow(windowName);
            cout << "Slice seleccionado: #" << sliceActual << endl;
            return sliceActual;
        }
        else if (key == 27) { // ESC
            destroyWindow(windowName);
            exit(0);
        }
    }
}

void mostrarDenoising(const Mat& original, const Mat& denoised_gaussian, const Mat& denoised_ia) {
    const string windowName = "Comparacion Denoising";
    
    // Redimensionar imágenes para comparación
    Mat orig_resized, gauss_resized, ia_resized;
    // Aumentamos ligeramente el factor para ventanas más grandes
    double scale = 0.75;

    resize(original, orig_resized, Size(), scale, scale);
    resize(denoised_gaussian, gauss_resized, Size(), scale, scale);
    resize(denoised_ia, ia_resized, Size(), scale, scale);
    
    // Convertir a color
    Mat orig_color, gauss_color, ia_color;
    cvtColor(orig_resized, orig_color, COLOR_GRAY2BGR);
    cvtColor(gauss_resized, gauss_color, COLOR_GRAY2BGR);
    cvtColor(ia_resized, ia_color, COLOR_GRAY2BGR);
    
    // Agregar etiquetas
    putText(orig_color, "Original", Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
    putText(gauss_color, "Gaussiano", Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
    putText(ia_color, "DnCNN ", Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
    
    // Concatenar horizontalmente
    Mat comparacion;
    hconcat(vector<Mat>{orig_color, gauss_color, ia_color}, comparacion);
    
    // Agregar instrucciones
    string instrucciones = "Presiona 'S' para continuar";
    putText(comparacion, instrucciones, 
            Point(10, comparacion.rows - 20), 
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(255, 255, 0), 2);
    
    namedWindow(windowName, WINDOW_NORMAL);
    imshow(windowName, comparacion);
    // Ajustar ventana a la comparación (hasta un máximo razonable)
    int winW = std::min(1600, comparacion.cols);
    int winH = std::min(900, comparacion.rows);
    resizeWindow(windowName, winW, winH);

    esperarTeclaS("Revisa la comparación de denoising:");
    destroyWindow(windowName);
}

OpcionesSegmentacion seleccionarTiposSegmentacion() {
    OpcionesSegmentacion opciones;
    const string windowName = "Seleccionar Tipos de Segmentacion";

    // Ventana de menú redimensionable para mostrar más opciones
    namedWindow(windowName, WINDOW_NORMAL);

    // Crear imagen de menú (más grande para mejor legibilidad)
    Mat menu(600, 900, CV_8UC3, Scalar(40, 40, 40));
    
    cout << "\n========================================" << endl;
    cout << "SELECCIÓN DE TIPOS DE SEGMENTACIÓN" << endl;
    cout << "========================================" << endl;
    cout << "Controles:" << endl;
    cout << "  [1] : Toggle Pulmones" << endl;
    cout << "  [2] : Toggle Corazon" << endl;
    cout << "  [3] : Toggle Tejidos Blandos" << endl;
    cout << "  [4] : Toggle Huesos" << endl;
    cout << "  [S] : Confirmar selección" << endl;
    cout << "  [ESC] : Salir" << endl;
    
    while(true) {
        menu.setTo(Scalar(40, 40, 40));
        
        // Título
        putText(menu, "SELECCIONAR SEGMENTACION", Point(50, 40), 
                FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2);
        
        // Opciones
        int y = 100;
        int lineHeight = 60;
        
        // Pulmones
        Scalar color1 = opciones.pulmones ? Scalar(0, 255, 0) : Scalar(100, 100, 100);
        putText(menu, "[1] Pulmones", Point(50, y), FONT_HERSHEY_SIMPLEX, 0.7, color1, 2);
        if(opciones.pulmones) {
            putText(menu, " [X]", Point(250, y), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        }
        y += lineHeight;
        
        // Corazón
        Scalar color2 = opciones.corazon ? Scalar(0, 255, 0) : Scalar(100, 100, 100);
        putText(menu, "[2] Corazon", Point(50, y), FONT_HERSHEY_SIMPLEX, 0.7, color2, 2);
        if(opciones.corazon) {
            putText(menu, " [X]", Point(250, y), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        }
        y += lineHeight;
        
        // Tejidos Blandos
        Scalar color3 = opciones.tejidosBlandos ? Scalar(0, 255, 0) : Scalar(100, 100, 100);
        putText(menu, "[3] Tejidos Blandos", Point(50, y), FONT_HERSHEY_SIMPLEX, 0.7, color3, 2);
        if(opciones.tejidosBlandos) {
            putText(menu, " [X]", Point(250, y), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        }
        y += lineHeight;
        
        // Huesos
        Scalar color4 = opciones.huesos ? Scalar(0, 255, 0) : Scalar(100, 100, 100);
        putText(menu, "[4] Huesos", Point(50, y), FONT_HERSHEY_SIMPLEX, 0.7, color4, 2);
        if(opciones.huesos) {
            putText(menu, " [X]", Point(250, y), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        }
        
        // Instrucciones
        putText(menu, "Presiona [S] para confirmar", Point(50, 350), 
                FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 0), 1);
        
    imshow(windowName, menu);
    // Ajustar la ventana del menú a un tamaño fijo legible
    resizeWindow(windowName, 900, 600);
        
        int key = waitKey(0) & 0xFF;
        
        if(key == '1') {
            opciones.pulmones = !opciones.pulmones;
        }
        else if(key == '2') {
            opciones.corazon = !opciones.corazon;
        }
        else if(key == '3') {
            opciones.tejidosBlandos = !opciones.tejidosBlandos;
        }
        else if(key == '4') {
            opciones.huesos = !opciones.huesos;
        }
        else if(key == 's' || key == 'S') {
            // Verificar que al menos una opción esté seleccionada
            if(opciones.pulmones || opciones.corazon || opciones.tejidosBlandos || opciones.huesos) {
                destroyWindow(windowName);
                cout << "\nOpciones seleccionadas:" << endl;
                if(opciones.pulmones) cout << "  - Pulmones" << endl;
                if(opciones.corazon) cout << "  - Corazon" << endl;
                if(opciones.tejidosBlandos) cout << "  - Tejidos Blandos" << endl;
                if(opciones.huesos) cout << "  - Huesos" << endl;
                return opciones;
            } else {
                cout << "Debes seleccionar al menos una opcion!" << endl;
            }
        }
        else if(key == 27) { // ESC
            destroyWindow(windowName);
            exit(0);
        }
    }
}

void mostrarResultadoFinal(const Mat& imagenBase, 
                          const Mat& lungsMask, 
                          const Mat& heartMask,
                          const Mat& softTissueMask,
                          const Mat& bonesMask,
                          const OpcionesSegmentacion& opciones) {
    const string windowName = "Resultado Final - Areas Resaltadas";
    
    Mat resultado;
    cvtColor(imagenBase, resultado, COLOR_GRAY2BGR);
    
    // Aplicar máscaras según opciones seleccionadas
    if(opciones.pulmones && !lungsMask.empty()) {
        resultado.setTo(Scalar(255, 100, 100), lungsMask);  // Azul
    }
    if(opciones.corazon && !heartMask.empty()) {
        resultado.setTo(Scalar(100, 100, 255), heartMask);  // Rojo
    }
    if(opciones.tejidosBlandos && !softTissueMask.empty()) {
        resultado.setTo(Scalar(255, 200, 0), softTissueMask);  // Cyan
    }
    if(opciones.huesos && !bonesMask.empty()) {
        resultado.setTo(Scalar(100, 255, 100), bonesMask);  // Verde
    }
    
    // Agregar leyenda
    int y = 30;
    if(opciones.pulmones) {
        putText(resultado, "Azul: Pulmones", Point(10, y), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 100, 100), 2);
        y += 25;
    }
    if(opciones.corazon) {
        putText(resultado, "Rojo: Corazon", Point(10, y), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(100, 100, 255), 2);
        y += 25;
    }
    if(opciones.tejidosBlandos) {
        putText(resultado, "Cyan: Tejidos Blandos", Point(10, y), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 200, 0), 2);
        y += 25;
    }
    if(opciones.huesos) {
        putText(resultado, "Verde: Huesos", Point(10, y), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(100, 255, 100), 2);
    }
    
    // Instrucciones
    putText(resultado, "Presiona cualquier tecla para cerrar", 
            Point(10, resultado.rows - 20), 
            FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
    
    namedWindow(windowName, WINDOW_NORMAL);
    imshow(windowName, resultado);
    int winW = std::min(1600, resultado.cols);
    int winH = std::min(900, resultado.rows);
    resizeWindow(windowName, winW, winH);
    
    cout << "\n========================================" << endl;
    cout << "RESULTADO FINAL MOSTRADO" << endl;
    cout << "========================================" << endl;
    cout << "Presiona cualquier tecla para cerrar..." << endl;
    
    waitKey(0);
    destroyWindow(windowName);
}

