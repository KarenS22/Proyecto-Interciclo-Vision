import requests
import base64
import cv2
import numpy as np
import matplotlib.pyplot as plt

# 1. Configuración
URL = "http://127.0.0.1:5000/denoise"
# Asegúrate de que esta imagen exista o cambia el nombre
IMAGEN_PRUEBA = "01_original.png" 

# 2. Cargar imagen y convertir a Base64
img = cv2.imread(IMAGEN_PRUEBA, cv2.IMREAD_GRAYSCALE) 

if img is None:
    print(f"Error: No encuentro la imagen {IMAGEN_PRUEBA}")
    exit()

# Codificamos a PNG en memoria -> Bytes -> Base64 string
_, buffer = cv2.imencode('.png', img)
img_base64 = base64.b64encode(buffer).decode('utf-8')

# 3. Enviar al servidor
print(f"Enviando imagen {img.shape} al servidor...")
payload = {"image": img_base64}

try:
    response = requests.post(URL, json=payload)
    
    if response.status_code == 200:
        data = response.json()
        print("¡Éxito! Imagen procesada recibida.")
        
        # 4. Decodificar respuesta
        processed_b64 = data["processed_image"]
        processed_bytes = base64.b64decode(processed_b64)
        nparr = np.frombuffer(processed_bytes, np.uint8)
        img_denoised = cv2.imdecode(nparr, cv2.IMREAD_GRAYSCALE)

        # ---------------------------------------------------------
        # 5. CALCULAR LA DIFERENCIA (El ruido eliminado)
        # ---------------------------------------------------------
        # Convertimos a int16 para evitar desbordamiento en la resta y luego valor absoluto
        diff = cv2.absdiff(img, img_denoised)
        
        # Amplificamos la diferencia visualmente (x5) para ver mejor el ruido tenue
        diff_enhanced = cv2.multiply(diff, 5) 

        # ---------------------------------------------------------
        # 6. VISUALIZACIÓN PROFESIONAL
        # ---------------------------------------------------------
        plt.figure(figsize=(15, 6))

        # Original
        plt.subplot(1, 3, 1)
        plt.title("1. Original (Con Ruido)")
        plt.imshow(img, cmap='gray')
        plt.axis('off')
        
        # Procesada
        plt.subplot(1, 3, 2)
        plt.title("2. Procesada (DnCNN-B)")
        plt.imshow(img_denoised, cmap='gray')
        plt.axis('off')

        # Diferencia (Lo que se quitó)
        plt.subplot(1, 3, 3)
        plt.title("3. Ruido Eliminado (Diferencia x5)")
        plt.imshow(diff_enhanced, cmap='inferno') # 'inferno' resalta mejor el ruido
        plt.axis('off')

        plt.tight_layout()
        plt.show()
        
    else:
        print("Error del servidor:", response.text)

except Exception as e:
    print("No se pudo conectar:", e)