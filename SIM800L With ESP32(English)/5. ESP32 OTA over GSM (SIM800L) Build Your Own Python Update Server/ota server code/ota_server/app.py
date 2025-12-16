from flask import Flask, request, render_template, send_from_directory, redirect, url_for
import os

app = Flask(__name__)

# Configuration
UPLOAD_FOLDER = 'firmware'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Ensure firmware directory exists
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

# --- Web Interface Routes ---

@app.route('/')
def index():
    # Read current version to display on the page
    version_path = os.path.join(app.config['UPLOAD_FOLDER'], 'version.txt')
    current_version = "Unknown"
    if os.path.exists(version_path):
        with open(version_path, 'r') as f:
            current_version = f.read().strip()
            
    return render_template('index.html', version=current_version)

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'firmware' not in request.files:
        return "No file part", 400
    
    file = request.files['firmware']
    new_version = request.form.get('version')

    if file.filename == '':
        return "No selected file", 400

    if file and new_version:
        # Save the binary file always as 'firmware.bin' to keep ESP32 URL constant
        bin_path = os.path.join(app.config['UPLOAD_FOLDER'], 'firmware.bin')
        file.save(bin_path)

        # Save the version number
        ver_path = os.path.join(app.config['UPLOAD_FOLDER'], 'version.txt')
        with open(ver_path, 'w') as f:
            f.write(new_version)

        return redirect(url_for('index', status='success'))
    
    return "Error saving file", 500

# --- ESP32 API Routes ---

@app.route('/api/version')
def get_version():
    """ ESP32 checks this URL to see if update is needed """
    return send_from_directory(app.config['UPLOAD_FOLDER'], 'version.txt')

@app.route('/api/firmware.bin')
def get_firmware():
    """ ESP32 downloads the file from here """
    return send_from_directory(app.config['UPLOAD_FOLDER'], 'firmware.bin')

if __name__ == '__main__':
    # Host '0.0.0.0' allows external access (Critical for VPS)
    app.run(host='0.0.0.0', port=5000)