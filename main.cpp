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
#include "Interfaz.hpp"
#include "Pulmones.hpp"

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

    // Creamos las ventanas (redimensionables para visualizar con mayor tamaño)
    namedWindow("Calibrando Tejidos", WINDOW_NORMAL);
    namedWindow("Visualizacion Color", WINDOW_NORMAL);

    // Creamos los Trackbars (Perillas)
    createTrackbar("Min Gray", "Calibrando Tejidos", &g_min_gray, 255, on_trackbar_tejidos);
    createTrackbar("Max Gray", "Calibrando Tejidos", &g_max_gray, 255, on_trackbar_tejidos);
    createTrackbar("Limpieza (Kernel)", "Calibrando Tejidos", &g_morph_size, 30, on_trackbar_tejidos);

    // Llamada inicial para que se vea algo
    on_trackbar_tejidos(0, 0);

    // Ajustar tamaño de ventanas para mejor visibilidad
    resizeWindow("Calibrando Tejidos", 1200, 700);
    resizeWindow("Visualizacion Color", 1200, 700);

    cout << "\n🎚️ MODO CALIBRACIÓN ACTIVADO" << endl;
    cout << "   - Ajusta 'Min Gray' y 'Max Gray' para atrapar el corazón." << endl;
    cout << "   - Ajusta 'Limpieza' para borrar el ruido." << endl;
    cout << "   - Presiona ESPACIO para guardar los valores y continuar." << endl;

    // Pausar hasta que presiones tecla
    waitKey(0);

    cout << "✅ VALORES GUARDADOS: " << endl;
    cout << "   inRange(" << g_min_gray << ", " << g_max_gray << ")" << endl;
    cout << "   Kernel Size: " << g_morph_size << endl;

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
    
    // Selección interactiva (los slices se cargan sobre la marcha)
    int sliceNum = seleccionarSlice(image3D, minSlice, maxSlice);
    
    auto start_slice = chrono::high_resolution_clock::now();
    
    string sliceFolder = "output/slice_" + to_string(sliceNum);
    fs::create_directories(sliceFolder + "/comparaciones");
    
    // Extraer el slice seleccionado
    Mat original = itkSliceToMat(image3D, sliceNum);
    imwrite(sliceFolder + "/01_original.png", original);
    cout << "\n✓ Slice #" << sliceNum << " seleccionado y guardado" << endl;
        
    // ==========================================================
    // FASE 2: DENOISING (DOS MÉTODOS EN PARALELO)
    // ==========================================================
    cout << "\n========================================" << endl;
    cout << "FASE 2: REDUCCIÓN DE RUIDO" << endl;
    cout << "========================================" << endl;
    
    // MÉTODO A: Denoising clásico (OpenCV)
    cout << "Aplicando filtro Gaussiano..." << endl;
    Mat denoised_gaussian;
    GaussianBlur(original, denoised_gaussian, Size(5, 5), 1.5);
    imwrite(sliceFolder + "/02a_denoised_gaussian.png", denoised_gaussian);
    
    // MÉTODO B: Red Neuronal (DnCNN via Flask)
    cout << "Aplicando DnCNN (IA)..." << endl;
    FlaskResponse flaskResp = enviarAFlask(original);
    Mat denoised_ia = flaskResp.imagen;
    imwrite(sliceFolder + "/02b_denoised_DnCNN.png", denoised_ia);
    
    if(flaskResp.success) {
        cout << "✓ DnCNN completado" << endl;
    }
    
    // COMPARACIÓN VISUAL INTERACTIVA
    mostrarDenoising(original, denoised_gaussian, denoised_ia);
    
    // Guardar comparación
    Mat comp_denoising;
    vector<Mat> denoising_methods = {original, denoised_gaussian, denoised_ia};
    hconcat(denoising_methods, comp_denoising);
    imwrite(sliceFolder + "/comparaciones/denoising_todos.png", comp_denoising);
    
    // ELEGIR EL MEJOR (en este caso, DnCNN)
    Mat imagen_trabajo = denoised_ia.clone();
        
    // ==========================================================
    // FASE 3: PREPROCESAMIENTO (Mejorar contraste)
    // ==========================================================
    cout << "\n========================================" << endl;
    cout << "FASE 3: PREPROCESAMIENTO" << endl;
    cout << "========================================" << endl;
        
        // Contrast Stretching
        Mat stretched;
        double minVal, maxVal;
        minMaxLoc(imagen_trabajo, &minVal, &maxVal);
        imagen_trabajo.convertTo(stretched, CV_8U, 255.0/(maxVal - minVal), -minVal * 255.0/(maxVal - minVal));
        imwrite(sliceFolder + "/03_contrast_stretched.png", stretched);
        cout << "  ✓ Contrast Stretching" << endl;
        
        // CLAHE 
        Mat clahe_result;
        Ptr<CLAHE> clahe = createCLAHE(4.0, Size(8, 8));
        clahe->apply(stretched, clahe_result);
        imwrite(sliceFolder + "/05_clahe.png", clahe_result);
        cout << "  ✓ CLAHE " << endl;
    
        
        // Usar CLAHE para continuar
        imagen_trabajo = clahe_result.clone();
        
    // ==========================================================
    // FASE 4: TÉCNICAS PARA FACILITAR SEGMENTACIÓN
    // ==========================================================
    cout << "\n========================================" << endl;
    cout << "FASE 4: PREPARACIÓN PARA SEGMENTACIÓN" << endl;
    cout << "========================================" << endl;
        
        // Suavizado suave (para evitar ruido en bordes)
        Mat suavizado;
        GaussianBlur(imagen_trabajo, suavizado, Size(3, 3), 0.7);
        imwrite(sliceFolder + "/06_suavizado_segmentacion.png", suavizado);
        cout << "  ✓ Suavizado ligero" << endl;
        
        // Detección de bordes (para análisis, no para segmentación directa)
        // Mat edges;
        // Mat temp_blur;
        // GaussianBlur(denoised_ia, temp_blur, Size(5,5), 1.5); // Un blur extra ayuda a Canny
        // Canny(temp_blur, edges, 30, 100); // Umbrales un poco más bajos
        // imwrite(sliceFolder + "/07_canny_edges.png", edges);
        // cout << "  ✓ Detección de bordes (Canny sobre imagen limpia)" << endl;
        
        // Binarización (Otsu)
        // Mat binary;
        // threshold(suavizado, binary, 0, 255, THRESH_BINARY + THRESH_OTSU);
        // imwrite(sliceFolder + "/08_binary_otsu.png", binary);
        // cout << "  ✓ Binarización (Otsu)" << endl;
        
        // // Operaciones morfológicas (limpiar)
        // Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        // Mat morphed;
        // morphologyEx(binary, morphed, MORPH_OPEN, kernel);
        // imwrite(sliceFolder + "/09_morph_open.png", morphed);
        // cout << "  ✓ Morfología (open)" << endl;
        
        // morphologyEx(morphed, morphed, MORPH_CLOSE, kernel);
        // imwrite(sliceFolder + "/10_morph_close.png", morphed);
        // cout << "  ✓ Morfología (close)" << endl;

        // bitwise_not(morphed, morphed);
        // imwrite(sliceFolder + "/10b_morph_inverted.png", morphed);
        // cout << "  ✓ Inversión de máscara" << endl;
        
        // ==========================================================
        // FASE 5: MANIPULACIÓN DIRECTA DE PÍXELES (Demostración)
        // ==========================================================
        // cout << "\n[FASE 5] MANIPULACIÓN DIRECTA DE PÍXELES" << endl;
        // Mat manipulated = original.clone();
        
        // // Zona central: +30 brillo
        // for(int y = 200; y < 312 && y < manipulated.rows; y++) {
        //     for(int x = 200; x < 312 && x < manipulated.cols; x++) {
        //         int valor = manipulated.at<uchar>(y, x);
        //         manipulated.at<uchar>(y, x) = min(255, valor + 30);
        //     }
        // }
        
        // // Esquina: invertir
        // for(int y = 0; y < 100 && y < manipulated.rows; y++) {
        //     for(int x = 0; x < 100 && x < manipulated.cols; x++) {
        //         manipulated.at<uchar>(y, x) = 255 - manipulated.at<uchar>(y, x);
        //     }
        // }
        
        // imwrite(sliceFolder + "/11_pixel_manipulation.png", manipulated);
        // cout << "  ✓ Píxeles manipulados (demostración)" << endl;
        
    // ==========================================================
    // FASE 5: SELECCIÓN DE TIPOS DE SEGMENTACIÓN
    // ==========================================================
    OpcionesSegmentacion opciones = seleccionarTiposSegmentacion();
    
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
        if(opciones.pulmones) {
            heartMask = segmentarCorazon(suavizado, lungsMask);
        } else {
            Mat emptyMask = Mat::zeros(suavizado.size(), CV_8UC1);
            heartMask = segmentarCorazon(suavizado, emptyMask);
        }
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
        bonesMask = segmentarHuesos(suavizado);
        imwrite(sliceFolder + "/15_huesos_mask.png", bonesMask);
        areaBones = countNonZero(bonesMask);
        cout << "  ✓ Huesos segmentados (área=" << areaBones << " px)" << endl;
    }
        
    // ==========================================================
    // FASE 7: IMAGEN FINAL CON ÁREAS RESALTADAS
    // ==========================================================
    cout << "\n========================================" << endl;
    cout << "FASE 7: RESULTADO FINAL" << endl;
    cout << "========================================" << endl;
    
    // Mostrar resultado interactivo
    mostrarResultadoFinal(imagen_trabajo, lungsMask, heartMask, softTissueMask, bonesMask, opciones);
    
    // Guardar resultado
    Mat colorResult;
    cvtColor(imagen_trabajo, colorResult, COLOR_GRAY2BGR);
    
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
    metricsFile << sliceNum << ","
                << (flaskResp.success ? flaskResp.psnr : 0) << ","
                << (flaskResp.success ? flaskResp.ssim : 0) << ","
                << (flaskResp.success ? flaskResp.noise_std : 0) << ","
                << areaLungs << ","
                << areaHeart << ","
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
