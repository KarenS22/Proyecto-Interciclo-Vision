#include "FlaskClient.hpp"
#include "Base64.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <exception>

using json = nlohmann::json;
using namespace cv;
using namespace std;

// Callback para CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

FlaskResponse enviarAFlask(Mat imgOriginal) {
    FlaskResponse response;
    response.success = false;
    
    if (imgOriginal.empty()) {
        response.imagen = imgOriginal;
        return response;
    }

    // Codificar imagen
    vector<uchar> buf;
    imencode(".png", imgOriginal, buf);
    string img_base64 = base64_encode(buf.data(), buf.size());

    // Preparar JSON
    json payload;
    payload["image"] = img_base64;
    string json_str = payload.dump();

    // Enviar con CURL
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/denoise");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        cout << "    >>> Enviando a Flask (DnCNN)..." << flush;
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            cerr << "Error: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            response.imagen = imgOriginal;
            return response;
        }
        curl_easy_cleanup(curl);
    } else {
        response.imagen = imgOriginal;
        return response;
    }

    // Decodificar respuesta
    try {
        auto response_json = json::parse(readBuffer);
        
        if (response_json.contains("error")) {
            cout << "Error Flask: " << response_json["error"] << endl;
            response.imagen = imgOriginal;
            return response;
        }
        
        string processed_b64 = response_json["processed_image"];
        vector<uchar> decoded_data = base64_decode(processed_b64);
        Mat imgDenoised = imdecode(decoded_data, IMREAD_GRAYSCALE);
        
        // Extraer métricas si existen
        if(response_json.contains("metrics")) {
            response.psnr = response_json["metrics"]["psnr"];
            response.ssim = response_json["metrics"]["ssim"];
            response.noise_std = response_json["metrics"]["noise_std"];
        }
        
        response.imagen = imgDenoised;
        response.success = true;
        cout << "OK" << endl;
        
        return response;

    } catch (exception& e) {
        cerr << "Error JSON: " << e.what() << endl;
        response.imagen = imgOriginal;
        return response;
    }
}


