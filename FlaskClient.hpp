#ifndef FLASK_CLIENT_HPP
#define FLASK_CLIENT_HPP

#include <opencv2/opencv.hpp>
#include <string>

struct FlaskResponse {
    cv::Mat imagen;
    double psnr;
    double ssim;
    double noise_std;
    bool success;
    
    FlaskResponse() : psnr(0.0), ssim(0.0), noise_std(0.0), success(false) {}
};

// Función para enviar imagen a servidor Flask y obtener resultado de DnCNN
FlaskResponse enviarAFlask(cv::Mat imgOriginal);

#endif // FLASK_CLIENT_HPP


