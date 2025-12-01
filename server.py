from flask import Flask, request, jsonify
import cv2
import numpy as np
import base64
import torch
import torch.nn as nn
import os

# ---------------------------------------------------------
# CONFIGURACIÓN
# ---------------------------------------------------------
# Asegúrate de descargar el archivo 'net.pth' de la carpeta DnCNN-B
# y renombrarlo o cambiar esta ruta:
MODEL_PATH = "net.pth" 
DEVICE = "cuda" if torch.cuda.is_available() else "cpu"

print(f"--- Iniciando configuración en: {DEVICE} ---")

# ---------------------------------------------------------
# DEFINICIÓN DEL MODELO DnCNN
# ---------------------------------------------------------
class DnCNN(nn.Module):
    def __init__(self, channels=1, num_of_layers=20):
        # NOTA: DnCNN-B (Blind) usa 20 capas por defecto en el paper y el repo.
        # DnCNN-S (Specific) usa 17 capas.
        super(DnCNN, self).__init__()
        kernel_size = 3
        padding = 1
        features = 64
        
        layers = []
        
        # 1. Capa de entrada (Conv + ReLU)
        layers.append(nn.Conv2d(in_channels=channels, out_channels=features, 
                                kernel_size=kernel_size, padding=padding, bias=False))
        layers.append(nn.ReLU(inplace=True))
        
        # 2. Capas intermedias (Conv + BN + ReLU)
        # Para DnCNN-B, esto se repite 18 veces (total layers 20)
        for _ in range(num_of_layers - 2):
            layers.append(nn.Conv2d(in_channels=features, out_channels=features, 
                                    kernel_size=kernel_size, padding=padding, bias=False))
            layers.append(nn.BatchNorm2d(features))
            layers.append(nn.ReLU(inplace=True))
            
        # 3. Capa de salida (Conv) -> Predice el RUIDO (Residual)
        layers.append(nn.Conv2d(in_channels=features, out_channels=channels, 
                                kernel_size=kernel_size, padding=padding, bias=False))
        
        self.dncnn = nn.Sequential(*layers)

    def forward(self, x):
        # La red predice el ruido (residual) "y"
        noise = self.dncnn(x)
        # Retornamos el ruido para restarlo después, o restamos aquí.
        # Para mayor claridad y debugging, restamos aquí mismo:
        return x - noise 

# ---------------------------------------------------------
# CARGA DEL MODELO
# ---------------------------------------------------------
def load_model():
    # Inicializamos con 20 capas para el modelo BLIND (B)
    model = DnCNN(channels=1, num_of_layers=20)
    
    if not os.path.exists(MODEL_PATH):
        print(f"ERROR: No se encuentra el archivo de pesos {MODEL_PATH}")
        return None

    try:
        # Cargamos los pesos
        state_dict = torch.load(MODEL_PATH, map_location=DEVICE)
        
        # --- FIX PARA DATA PARALLEL ---
        # A veces los modelos entrenados en multi-GPU guardan las llaves con prefijo "module."
        # Esto las limpia para que encajen en un modelo de single-GPU.
        new_state_dict = {}
        for k, v in state_dict.items():
            name = k[7:] if k.startswith('module.') else k
            new_state_dict[name] = v
            
        model.load_state_dict(new_state_dict)
        model.to(DEVICE)
        model.eval() # Importante: modo evaluación para congelar Batch Norm
        print("Modelo DnCNN-B cargado exitosamente.")
        return model
    except Exception as e:
        print(f"Error cargando el modelo: {e}")
        return None

# Instancia global del modelo
net = load_model()

# ---------------------------------------------------------
# SERVIDOR FLASK
# ---------------------------------------------------------
app = Flask(__name__)

@app.route('/denoise', methods=['POST'])
def denoise_ct():
    if net is None:
        return jsonify({"error": "El modelo no está cargado en el servidor"}), 500

    try:
        data = request.json
        if "image" not in data:
            return jsonify({"error": "Falta el campo 'image'"}), 400
            
        image_b64 = data["image"]

        # 1. Decodificar Base64 a bytes
        img_bytes = base64.b64decode(image_b64)
        nparr = np.frombuffer(img_bytes, np.uint8)
        
        # 2. Leer como escala de grises (1 canal)
        # OJO: Aquí asumimos que el CT ya viene ventaneado (0-255)
        img = cv2.imdecode(nparr, cv2.IMREAD_GRAYSCALE)

        if img is None:
            return jsonify({"error": "No se pudo decodificar la imagen con OpenCV"}), 400

        # 3. Preprocesamiento: Normalizar [0, 1] y convertir a Tensor
        # Dimensiones: (Batch, Channel, Height, Width) -> (1, 1, H, W)
        img_norm = img.astype(np.float32) / 255.0
        img_tensor = torch.from_numpy(img_norm).unsqueeze(0).unsqueeze(0).to(DEVICE)

        # 4. Inferencia
        with torch.no_grad():
            clean_tensor = net(img_tensor)
        
        # 5. Postprocesamiento: Tensor a Numpy
        clean_np = clean_tensor.squeeze().cpu().numpy() # Quitar dim de batch y canal
        
        # Asegurar rango [0, 1] y convertir a uint8 [0, 255]
        clean_np = np.clip(clean_np, 0, 1)
        out_img = (clean_np * 255).astype(np.uint8)

        # 6. Codificar respuesta a Base64
        _, buffer = cv2.imencode('.png', out_img)
        out_b64 = base64.b64encode(buffer).decode('utf-8')

        return jsonify({
            "status": "success",
            "processed_image": out_b64,
            "original_shape": img.shape
        })

    except Exception as e:
        print(f"INTERNAL ERROR: {str(e)}")
        return jsonify({"error": str(e)}), 500

if __name__ == "__main__":
    # Ejecutar en todas las interfaces de red
    app.run(host="0.0.0.0", port=5000, debug=True)