#ifndef TIPOS_HPP
#define TIPOS_HPP

#include <itkImage.h>

typedef signed short InputPixelType;
constexpr unsigned int Dimension = 3;

typedef itk::Image<InputPixelType, Dimension> InputImageType;

#endif // TIPOS_HPP
