#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>

// Headers propios
#include "Operaciones.hpp"
#include "FlaskClient.hpp"
#include "InterfazIntegrada.hpp"
#include "Pulmones.hpp"
#include "Huesos.hpp"
#include "Corazon.hpp"

namespace fs = std::filesystem;
using namespace std;
using namespace cv;


// ============================================================================
// DEBUGGER DE TEJIDOS (TRACKBARS)
// ============================================================================

// ============================================================================
// VARIABLES GLOBALES PARA LOS SLIDERS
// ============================================================================
Mat g_img_suavizada; // Aquí guardaremos la imagen 06
int g_min_gray = 50;   // Umbral Mínimo
int g_max_gray = 200;  // Umbral Máximo
int g_morph_size = 5;  // Tamaño del Kernel para limpieza (1 a 20)

// ============================================================================
// CALLBACK (SE EJECUTA CUANDO MUEVES UN SLIDER)
// ============================================================================
void on_trackbar_tejidos(int, void*) {
    if (g_img_suavizada.empty()) return;

    // 1. UMBRALIZACIÓN DE RANGO (Lo que quieres probar)
    Mat mask;
    inRange(g_img_suavizada, Scalar(g_min_gray), Scalar(g_max_gray), mask);

    // 2. MORFOLOGÍA (El "otro" parámetro útil)
    // El tamaño del kernel define qué tan agresivo es el cierre de huecos
    // Aseguramos que sea impar y al menos 1
    int k_size = (g_morph_size < 1) ? 1 : g_morph_size;
    
    Mat cleaned;
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(k_size, k_size));
    
    // Aplicamos OPEN (quitar ruido) y CLOSE (cerrar huecos)
    morphologyEx(mask, cleaned, MORPH_OPEN, kernel);
    morphologyEx(cleaned, cleaned, MORPH_CLOSE, kernel);

    // 3. VISUALIZACIÓN
    // Mostramos la máscara binaria (Blanco/Negro)
    imshow("Calibrando Tejidos", cleaned);
    
    // Opcional: Mostrar sobre la original en verde para ver qué agarra
    Mat visualizacion;
    cvtColor(g_img_suavizada, visualizacion, COLOR_GRAY2BGR);
    Mat capaVerde(visualizacion.size(), CV_8UC3, Scalar(0, 255, 0));
    capaVerde.copyTo(visualizacion, cleaned); // Pintar solo la máscara
    addWeighted(visualizacion, 1.0, capaVerde, 0.3, 0, visualizacion); // Transparencia
    
    imshow("Visualizacion Color", visualizacion);
}

// ============================================================================
// FUNCIÓN PARA LLAMAR DESDE EL MAIN
// ============================================================================
void abrirCalibrador(Mat input) {
    g_img_suavizada = input.clone();

    // Ventanas redimensionables
    namedWindow("Calibrando Tejidos", WINDOW_NORMAL);
    namedWindow("Visualizacion Color", WINDOW_NORMAL);

    // Trackbars para navegación y parámetros
    static int view_mode = 0; // 0 = overlay, 1 = original, 2 = mask
    createTrackbar("Min Gray", "Calibrando Tejidos", &g_min_gray, 255, on_trackbar_tejidos);
    createTrackbar("Max Gray", "Calibrando Tejidos", &g_max_gray, 255, on_trackbar_tejidos);
    createTrackbar("Limpieza (Kernel)", "Calibrando Tejidos", &g_morph_size, 30, on_trackbar_tejidos);
    createTrackbar("Vista", "Calibrando Tejidos", &view_mode, 2, nullptr);

    // Ajustar tamaño por defecto
    resizeWindow("Calibrando Tejidos", 1200, 700);
    resizeWindow("Visualizacion Color", 1200, 700);

    cout << "\n🎚️ MODO CALIBRACIÓN (INTERACTIVO)" << endl;
    cout << "   - Usa los sliders para ajustar Min/Max/Limpieza." << endl;
    cout << "   - Presiona 1/2/3/4 para activar opciones de segmentación." << endl;
    cout << "   - Presiona S para guardar y salir, ESC para cancelar." << endl;

    OpcionesSegmentacion opciones; // mostrar estado en pantalla

    int key = 0;
    while (true) {
        if (g_img_suavizada.empty()) break;

        // Generar máscara según sliders
        Mat mask;
        inRange(g_img_suavizada, Scalar(g_min_gray), Scalar(g_max_gray), mask);

        int k_size = (g_morph_size < 1) ? 1 : g_morph_size;
        Mat cleaned;
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(k_size, k_size));
        morphologyEx(mask, cleaned, MORPH_OPEN, kernel);
        morphologyEx(cleaned, cleaned, MORPH_CLOSE, kernel);

        // Mostrar máscara limpia en la ventana de calibrado
        imshow("Calibrando Tejidos", cleaned);

        // Preparar visualizacion color
        Mat visualizacion;
        cvtColor(g_img_suavizada, visualizacion, COLOR_GRAY2BGR);

        Mat capaVerde(visualizacion.size(), CV_8UC3, Scalar(0, 255, 0));
        if (view_mode == 0) {
            // overlay
            capaVerde.copyTo(visualizacion, cleaned);
            addWeighted(visualizacion, 1.0, capaVerde, 0.3, 0, visualizacion);
        } else if (view_mode == 1) {
            // original (no overlay) - already in visualizacion
        } else {
            // mostrar la máscara como imagen coloreada
            Mat colorMask;
            cvtColor(cleaned, colorMask, COLOR_GRAY2BGR);
            visualizacion = colorMask;
        }

        // Mostrar estados de opciones de segmentación en la parte inferior
        string opts = string("1:Pulmones[") + (opciones.pulmones?"X":" ") + "]  ";
        opts += string("2:Corazon[") + (opciones.corazon?"X":" ") + "]  ";
        opts += string("3:Tejidos[") + (opciones.tejidosBlandos?"X":" ") + "]  ";
        opts += string("4:Huesos[") + (opciones.huesos?"X":" ") + "]  ";
        opts += "  S:Guardar  ESC:Salir";

        putText(visualizacion, opts, Point(10, visualizacion.rows - 10), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255,255,255), 1);

        imshow("Visualizacion Color", visualizacion);

        // Asegurar tamaños constantes de ventana
        resizeWindow("Calibrando Tejidos", 1200, 700);
        resizeWindow("Visualizacion Color", 1200, 700);

        key = waitKey(50) & 0xFF;
        if (key != 255) {
            if (key == '1') opciones.pulmones = !opciones.pulmones;
            else if (key == '2') opciones.corazon = !opciones.corazon;
            else if (key == '3') opciones.tejidosBlandos = !opciones.tejidosBlandos;
            else if (key == '4') opciones.huesos = !opciones.huesos;
            else if (key == 's' || key == 'S') {
                cout << "✅ VALORES GUARDADOS: " << endl;
                cout << "   inRange(" << g_min_gray << ", " << g_max_gray << ")" << endl;
                cout << "   Kernel Size: " << g_morph_size << endl;
                break;
            }
            else if (key == 27) { // ESC
                cout << "Proceso cancelado por el usuario." << endl;
                destroyWindow("Calibrando Tejidos");
                destroyWindow("Visualizacion Color");
                exit(0);
            }
        }
    }

    destroyWindow("Calibrando Tejidos");
    destroyWindow("Visualizacion Color");
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    
    if(argc < 2) {
        cerr << "Uso: " << argv[0] << " <ruta_carpeta_dicom> [slice1,slice2,slice3...]" << endl;
        cerr << "Ejemplo: " << argv[0] << " /path/to/L506/ 60,90,110" << endl;
        return -1;
    }
    
    string dicomDir = argv[1];
    vector<int> slicesToProcess;
    
    if(argc >= 3) {
        string slicesStr = argv[2];
        stringstream ss(slicesStr);
        string item;
        while(getline(ss, item, ',')) {
            slicesToProcess.push_back(stoi(item));
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "  PROCESADOR DE CT SCAN - COMPLETO     " << endl;
    cout << "========================================" << endl;
    cout << "Directorio: " << dicomDir << endl << endl;
    
    auto start_total = chrono::high_resolution_clock::now();
    
    // ===== LEER DICOM =====
    cout << "[1/9] Leyendo serie DICOM..." << endl;
    
    typedef itk::ImageSeriesReader<InputImageType> ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    typedef itk::GDCMImageIO ImageIOType;
    ImageIOType::Pointer dicomIO = ImageIOType::New();
    reader->SetImageIO(dicomIO);
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetUseSeriesDetails(true);
    nameGenerator->SetDirectory(dicomDir);
    
    const vector<string> & seriesUID = nameGenerator->GetSeriesUIDs();
    if(seriesUID.empty()) {
        cerr << "❌ No se encontraron series DICOM" << endl;
        return -1;
    }
    
    string seriesIdentifier = seriesUID.begin()->c_str();
    vector<string> fileNames = nameGenerator->GetFileNames(seriesIdentifier);
    reader->SetFileNames(fileNames);
    
    try {
        reader->Update();
    } catch(itk::ExceptionObject & ex) {
        cerr << "❌ Error: " << ex << endl;
        return -1;
    }
    
    InputImageType::Pointer image3D = reader->GetOutput();
    InputImageType::RegionType region = image3D->GetLargestPossibleRegion();
    InputImageType::SizeType size = region.GetSize();
    
    cout << "  ✓ Archivos: " << fileNames.size() << endl;
    cout << "  ✓ Dimensiones: " << size[0] << " x " << size[1] << " x " << size[2] << endl;
    
    // ==========================================================
    // FASE 1: SELECCIÓN INTERACTIVA DE SLICE (195-210)
    // ==========================================================
    const int minSlice = 195;
    const int maxSlice = 210;
    
    if(minSlice < 0 || maxSlice >= (int)size[2]) {
        cerr << "Error: Rango de slices invalido. Volumen tiene " << size[2] << " slices." << endl;
        return -1;
    }
    
    cout << "\n========================================" << endl;
    cout << "SELECCIÓN DE SLICE (Rango: " << minSlice << "-" << maxSlice << ")" << endl;
    cout << "========================================" << endl;
    
    // Interfaz integrada: muestra slice con trackbar, técnicas a la derecha, controles abajo
    ResultadoInterfaz resultado = interfazIntegrada(image3D, minSlice, maxSlice);
    
    int sliceNum = resultado.sliceNum;
    OpcionesSegmentacion opciones = resultado.opciones;
    
    auto start_slice = chrono::high_resolution_clock::now();
    
    string sliceFolder = "output/slice_" + to_string(sliceNum);
    fs::create_directories(sliceFolder + "/comparaciones");
    
    // Guardar imágenes de preprocesamiento (ya calculadas en la interfaz)
    imwrite(sliceFolder + "/01_original.png", resultado.original);
    imwrite(sliceFolder + "/02a_denoised_gaussian.png", resultado.denoised_gaussian);
    imwrite(sliceFolder + "/02b_denoised_DnCNN.png", resultado.denoised_ia);
    imwrite(sliceFolder + "/03_contrast_stretched.png", resultado.stretched);
    imwrite(sliceFolder + "/05_clahe.png", resultado.clahe_result);
    imwrite(sliceFolder + "/06_suavizado_segmentacion.png", resultado.suavizado);
    
    // Guardar comparación
    Mat comp_denoising;
    vector<Mat> denoising_methods = {resultado.original, resultado.denoised_gaussian, resultado.denoised_ia};
    hconcat(denoising_methods, comp_denoising);
    imwrite(sliceFolder + "/comparaciones/denoising_todos.png", comp_denoising);
    
    cout << "\n✓ Slice #" << sliceNum << " procesado y guardado" << endl;
        
    // ==========================================================
    // FASE 2: SEGMENTACIÓN (Según opciones seleccionadas)
    // ==========================================================
    cout << "\n========================================" << endl;
    cout << "FASE 2: SEGMENTACIÓN" << endl;
    cout << "========================================" << endl;
    
    // Usar la imagen suavizada para segmentación
    Mat suavizado = resultado.suavizado.clone();
    
    // ==========================================================
    // FASE 6: SEGMENTACIÓN (Umbrales Hounsfield)
    // ==========================================================
    cout << "\n========================================" << endl;
    cout << "FASE 6: SEGMENTACIÓN" << endl;
    cout << "========================================" << endl;
    
    Mat lungsMask, heartMask, softTissueMask, bonesMask;
    int areaLungs = 0, areaHeart = 0, areaSoftTissue = 0, areaBones = 0;
    
    // Segmentar según opciones seleccionadas
    if(opciones.pulmones) {
        cout << "Segmentando pulmones..." << endl;
        lungsMask = mostrarPulmonesConSliders(suavizado);
        imwrite(sliceFolder + "/12_pulmones_mask.png", lungsMask);
        areaLungs = countNonZero(lungsMask);
        cout << "  ✓ Pulmones segmentados (área=" << areaLungs << " px)" << endl;
    }
    
    if(opciones.corazon) {
        cout << "Segmentando corazón..." << endl;
        heartMask = mostrarCorazonConSliders(suavizado);
        imwrite(sliceFolder + "/13_corazon_mask.png", heartMask);
        areaHeart = countNonZero(heartMask);
        cout << "  ✓ Corazón segmentado (área=" << areaHeart << " px)" << endl;
    }
    
    if(opciones.tejidosBlandos) {
        cout << "Segmentando tejidos blandos..." << endl;
        softTissueMask = segmentarTejidosBlandos(suavizado);
        imwrite(sliceFolder + "/14_tejidos_blandos_mask.png", softTissueMask);
        areaSoftTissue = countNonZero(softTissueMask);
        cout << "  ✓ Tejidos blandos segmentados (área=" << areaSoftTissue << " px)" << endl;
    }
    
    if(opciones.huesos) {
        cout << "Segmentando huesos..." << endl;
        bonesMask = mostrarHuesosConSliders(suavizado);
        imwrite(sliceFolder + "/15_huesos_mask.png", bonesMask);
        areaBones = countNonZero(bonesMask);
        cout << "  ✓ Huesos segmentados (área=" << areaBones << " px)" << endl;
    }
        
    // ==========================================================
    // FASE 3: IMAGEN FINAL CON ÁREAS RESALTADAS
    // ==========================================================
    cout << "\n========================================" << endl;
    cout << "FASE 3: RESULTADO FINAL" << endl;
    cout << "========================================" << endl;
    
    // Mostrar resultado interactivo
    mostrarResultadoFinal(resultado.clahe_result, lungsMask, heartMask, softTissueMask, bonesMask, opciones);
    
    // Guardar resultado
    Mat colorResult;
    cvtColor(resultado.clahe_result, colorResult, COLOR_GRAY2BGR);
    
    if(opciones.pulmones && !lungsMask.empty()) {
        colorResult.setTo(Scalar(255, 100, 100), lungsMask);  // Azul
    }
    if(opciones.corazon && !heartMask.empty()) {
        colorResult.setTo(Scalar(100, 100, 255), heartMask);  // Rojo
    }
    if(opciones.tejidosBlandos && !softTissueMask.empty()) {
        colorResult.setTo(Scalar(255, 200, 0), softTissueMask);  // Cyan
    }
    if(opciones.huesos && !bonesMask.empty()) {
        colorResult.setTo(Scalar(100, 255, 100), bonesMask);  // Verde
    }
    
    imwrite(sliceFolder + "/20_resultado_final.png", colorResult);
    cout << "✓ Imagen final guardada en: " << sliceFolder << "/20_resultado_final.png" << endl;
    
    auto end_slice = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_slice - start_slice);
    
    // Guardar métricas
    ofstream metricsFile("output/metricas.csv", ios::app);
    if(!metricsFile.is_open()) {
        metricsFile.open("output/metricas.csv", ios::out);
        metricsFile << "Slice,PSNR_dB,SSIM,Noise_STD,Area_Pulmones,Area_Corazon,Area_Tejidos,Area_Huesos,Tiempo_ms" << endl;
    }
    metricsFile << sliceNum << ","
                << "0" << ","  // PSNR (se puede calcular después)
                << "0" << ","  // SSIM
                << "0" << ","  // Noise_STD
                << areaLungs << ","
                << areaHeart << ","
                << areaSoftTissue << ","
                << areaBones << ","
                << duration.count() << endl;
    metricsFile.close();
    
    auto end_total = chrono::high_resolution_clock::now();
    auto duration_total = chrono::duration_cast<chrono::seconds>(end_total - start_total);
    
    cout << "\n========================================" << endl;
    cout << "PROCESAMIENTO COMPLETO" << endl;
    cout << "========================================" << endl;
    cout << "Slice procesado: #" << sliceNum << endl;
    cout << "Tiempo total: " << duration_total.count() << " segundos" << endl;
    cout << "Resultados en: " << sliceFolder << endl;
    cout << "Métricas en: output/metricas.csv" << endl;
    cout << endl;
    
    return 0;
}
