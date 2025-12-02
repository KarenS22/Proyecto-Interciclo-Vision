#include "InterfazIntegrada.hpp"
#include "Operaciones.hpp"
#include "FlaskClient.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace cv;
using namespace std;

void onTrackbarChange(int pos, void* userdata) {
    // Callback para trackbar
}

ResultadoInterfaz interfazIntegrada(InputImageType::Pointer image3D, int minSlice, int maxSlice) {
    ResultadoInterfaz resultado;
    OpcionesSegmentacion opciones;
    
    const string windowName = "Procesador CT Scan - Interfaz Integrada";
    namedWindow(windowName, WINDOW_NORMAL);
    
    cout << "\n========================================" << endl;
    cout << "INTERFAZ INTEGRADA - PROCESADOR CT SCAN" << endl;
    cout << "========================================" << endl;
    cout << "Controles:" << endl;
    cout << "  - Trackbar: Navegar entre slices (" << minSlice << "-" << maxSlice << ")" << endl;
    cout << "  - [1] : Toggle Pulmones" << endl;
    cout << "  - [2] : Toggle Corazon" << endl;
    cout << "  - [3] : Toggle Tejidos Blandos" << endl;
    cout << "  - [4] : Toggle Huesos" << endl;
    cout << "  - [S] : Confirmar y procesar" << endl;
    cout << "  - [ESC] : Salir" << endl;
    
    int trackPos = 0;
    int trackMax = maxSlice - minSlice;
    createTrackbar("Slice", windowName, &trackPos, trackMax, onTrackbarChange);
    
    // Variables para cachear resultados
    int lastSlice = -1;
    Mat cached_original, cached_denoised_gaussian, cached_denoised_ia;
    Mat cached_stretched, cached_clahe, cached_suavizado;
    
    while(true) {
        int sliceActual = minSlice + trackPos;
        
        // Si cambió el slice, recalcular todo
        if(sliceActual != lastSlice) {
            cout << "Procesando slice #" << sliceActual << "..." << endl;
            
            resultado.original = itkSliceToMat(image3D, sliceActual);
            cached_original = resultado.original.clone();
            
            GaussianBlur(resultado.original, resultado.denoised_gaussian, Size(5, 5), 1.5);
            cached_denoised_gaussian = resultado.denoised_gaussian.clone();
            
            static bool dncnn_calculado = false;
            static int dncnn_slice = -1;
            if(!dncnn_calculado || dncnn_slice != sliceActual) {
                cout << "  Aplicando DnCNN..." << flush;
                FlaskResponse flaskResp = enviarAFlask(resultado.original);
                if(flaskResp.success) {
                    resultado.denoised_ia = flaskResp.imagen;
                    cached_denoised_ia = resultado.denoised_ia.clone();
                    dncnn_calculado = true;
                    dncnn_slice = sliceActual;
                    cout << " OK" << endl;
                } else {
                    resultado.denoised_ia = resultado.denoised_gaussian.clone();
                    cached_denoised_ia = resultado.denoised_gaussian.clone();
                    cout << " (usando Gaussiano como fallback)" << endl;
                }
            } else {
                resultado.denoised_ia = cached_denoised_ia.clone();
            }
            
            Mat imagen_trabajo = resultado.denoised_ia.clone();
            double minVal, maxVal;
            minMaxLoc(imagen_trabajo, &minVal, &maxVal);
            imagen_trabajo.convertTo(resultado.stretched, CV_8U, 255.0/(maxVal - minVal), -minVal * 255.0/(maxVal - minVal));
            cached_stretched = resultado.stretched.clone();
            
            Ptr<CLAHE> clahe = createCLAHE(4.0, Size(8, 8));
            clahe->apply(resultado.stretched, resultado.clahe_result);
            cached_clahe = resultado.clahe_result.clone();
            
            GaussianBlur(resultado.clahe_result, resultado.suavizado, Size(3, 3), 0.7);
            cached_suavizado = resultado.suavizado.clone();
            
            lastSlice = sliceActual;
            resultado.sliceNum = sliceActual;
        } else {
            resultado.original = cached_original.clone();
            resultado.denoised_gaussian = cached_denoised_gaussian.clone();
            resultado.denoised_ia = cached_denoised_ia.clone();
            resultado.stretched = cached_stretched.clone();
            resultado.clahe_result = cached_clahe.clone();
            resultado.suavizado = cached_suavizado.clone();
        }
        
        // ==========================================================
        // CONSTRUIR INTERFAZ EN FORMATO MATRICIAL
        // ==========================================================
        
        int imgSize = 350;
        int spacing = 5;
        Size displaySize(imgSize, imgSize);
        
        // Preparar imágenes redimensionadas
        auto prepararImagen = [&](const Mat& src, const string& label) -> Mat {
            Mat resized, color;
            resize(src, resized, displaySize);
            
            if(resized.channels() == 1) {
                cvtColor(resized, color, COLOR_GRAY2BGR);
            } else {
                color = resized.clone();
            }
            
            putText(color, label, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 2);
            return color;
        };
        
        // Preparar imagen grande del slice original
        int bigSize = imgSize * 2;
        Size bigDisplaySize(bigSize, bigSize);
        Mat slice_grande, slice_color;
        resize(resultado.original, slice_grande, bigDisplaySize);
        
        if(slice_grande.channels() == 1) {
            cvtColor(slice_grande, slice_color, COLOR_GRAY2BGR);
        } else {
            slice_color = slice_grande.clone();
        }
        
        string sliceText = "ORIGINAL - Slice #" + to_string(sliceActual);
        putText(slice_color, sliceText, Point(20, 50), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 3);
        
        // Preparar todas las técnicas
        Mat t1 = prepararImagen(resultado.denoised_gaussian, "Blur Gaussiano");
        Mat t2 = prepararImagen(resultado.denoised_ia, "DnCNN Denoising");
        Mat t3 = prepararImagen(resultado.stretched, "Contrast Stretch");
        Mat t4 = prepararImagen(resultado.clahe_result, "CLAHE");
        Mat t5 = prepararImagen(resultado.suavizado, "Suavizado");
        
        // Crear panel de controles (mismo ancho que las técnicas)
        Mat panel_controles(imgSize, imgSize, CV_8UC3, Scalar(40, 40, 40));
        
        // Texto de controles (más compacto)
        vector<string> textos_controles = {
            "[1] " + string(opciones.pulmones ? "[X]" : "[ ]") + " Pulmones",
            "[2] " + string(opciones.corazon ? "[X]" : "[ ]") + " Corazon",
            // "[3] " + string(opciones.tejidosBlandos ? "[X]" : "[ ]") + " Tejidos",
            "[3] " + string(opciones.huesos ? "[X]" : "[ ]") + " Huesos",
            "",
            "[S] Confirmar",
            "[ESC] Salir"
        };
        
        int yPos = 20;
        for(const auto& texto : textos_controles) {
            if(!texto.empty()) {
                putText(panel_controles, texto, Point(10, yPos), 
                        FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
            }
            yPos += 25;
        }
        
        // LAYOUT MATRICIAL:
        // |-----------|-----|-----|-----|
        // | ORIGINAL  | T1  | T2  | T3  |
        // | (grande)  |-----|-----|-----|
        // |           | T4  | T5  |CTRL |
        // |-----------|-----|-----|-----|
        
        // Fila superior de técnicas (T1, T2, T3)
        Mat fila_superior_tecnicas;
        hconcat(vector<Mat>{t1, t2, t3}, fila_superior_tecnicas);
        
        // Fila inferior de técnicas (T4, T5, Controles)
        Mat fila_inferior_tecnicas;
        hconcat(vector<Mat>{t4, t5, panel_controles}, fila_inferior_tecnicas);
        
        // Columna derecha completa (ambas filas de técnicas)
        Mat columna_derecha;
        vconcat(vector<Mat>{fila_superior_tecnicas, fila_inferior_tecnicas}, columna_derecha);
        
        // Ajustar slice_color para que tenga la misma altura que columna_derecha
        Mat slice_ajustado;
        if(slice_color.rows != columna_derecha.rows) {
            resize(slice_color, slice_ajustado, Size(bigSize, columna_derecha.rows));
        } else {
            slice_ajustado = slice_color.clone();
        }
        
        // Combinar todo horizontalmente
        Mat interfaz_completa;
        hconcat(vector<Mat>{slice_ajustado, columna_derecha}, interfaz_completa);
        
        // Panel inferior con información adicional
        int panelHeight = 60;
        Mat panel_info(panelHeight, interfaz_completa.cols, CV_8UC3, Scalar(30, 30, 30));
        
        string sliceInfo = "Slice: " + to_string(sliceActual) + " / " + to_string(maxSlice) + 
                          "  |  Rango: " + to_string(minSlice) + "-" + to_string(maxSlice);
        putText(panel_info, sliceInfo, Point(20, 25), 
                FONT_HERSHEY_SIMPLEX, 0.6, Scalar(150, 200, 255), 2);
        
        string instruccion = "Usa el trackbar para navegar. Selecciona opciones con teclas 1-4. Presiona S para confirmar.";
        putText(panel_info, instruccion, Point(20, 45), 
                FONT_HERSHEY_SIMPLEX, 0.45, Scalar(200, 200, 200), 1);
        
        // Agregar panel inferior
        Mat interfaz_final;
        vconcat(vector<Mat>{interfaz_completa, panel_info}, interfaz_final);
        
        namedWindow(windowName, WINDOW_NORMAL);
        imshow(windowName, interfaz_final);
        resizeWindow(windowName, interfaz_final.cols, interfaz_final.rows);
        
        // Manejar teclas
        int key = waitKey(30) & 0xFF;
        if(key != 255 && key != -1) {
            if(key == '1') {
                opciones.pulmones = !opciones.pulmones;
                cout << "Pulmones: " << (opciones.pulmones ? "ON" : "OFF") << endl;
            }
            else if(key == '2') {
                opciones.corazon = !opciones.corazon;
                cout << "Corazon: " << (opciones.corazon ? "ON" : "OFF") << endl;
            }
            // else if(key == '3') {
            //     opciones.tejidosBlandos = !opciones.tejidosBlandos;
            //     cout << "Tejidos Blandos: " << (opciones.tejidosBlandos ? "ON" : "OFF") << endl;
            // }
            else if(key == '3') {
                opciones.huesos = !opciones.huesos;
                cout << "Huesos: " << (opciones.huesos ? "ON" : "OFF") << endl;
            }
            else if(key == 's' || key == 'S') {
                if(opciones.pulmones || opciones.corazon || opciones.tejidosBlandos || opciones.huesos) {
                    resultado.opciones = opciones;
                    destroyWindow(windowName);
                    cout << "\n========================================" << endl;
                    cout << "CONFIGURACION CONFIRMADA" << endl;
                    cout << "========================================" << endl;
                    cout << "Slice: #" << sliceActual << endl;
                    cout << "Opciones seleccionadas:" << endl;
                    if(opciones.pulmones) cout << "  - Pulmones" << endl;
                    if(opciones.corazon) cout << "  - Corazon" << endl;
                    // if(opciones.tejidosBlandos) cout << "  - Tejidos Blandos" << endl;
                    if(opciones.huesos) cout << "  - Huesos" << endl;
                    return resultado;
                } else {
                    cout << "Debes seleccionar al menos una opcion de segmentacion!" << endl;
                }
            }
            else if(key == 27) {
                destroyWindow(windowName);
                exit(0);
            }
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
    
    if(opciones.pulmones && !lungsMask.empty()) {
        resultado.setTo(Scalar(255, 100, 100), lungsMask);
    }
    if(opciones.corazon && !heartMask.empty()) {
        resultado.setTo(Scalar(100, 100, 255), heartMask);
    }
    if(opciones.tejidosBlandos && !softTissueMask.empty()) {
        resultado.setTo(Scalar(255, 200, 0), softTissueMask);
    }
    if(opciones.huesos && !bonesMask.empty()) {
        resultado.setTo(Scalar(100, 255, 100), bonesMask);
    }
    
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
    
    putText(resultado, "Presiona cualquier tecla para cerrar", 
            Point(10, resultado.rows - 20), 
            FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
    
    namedWindow(windowName, WINDOW_NORMAL);
    imshow(windowName, resultado);
    
    cout << "\n========================================" << endl;
    cout << "RESULTADO FINAL MOSTRADO" << endl;
    cout << "========================================" << endl;
    cout << "Presiona cualquier tecla para cerrar..." << endl;
    
    waitKey(0);
    destroyWindow(windowName);
}