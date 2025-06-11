# Techin515-Lab5

This project is a gesture recognition application built using TensorFlow and Flask. It processes gesture data and serves predictions via a web interface.

## Setup

1. **Clone the repository:**
   ```sh
   git clone https://github.com/prunus77/Techin515-Lab5.git
   cd Techin515-Lab5
   ```

2. **Create a virtual environment:**
   ```sh
   python3 -m venv venv
   source venv/bin/activate  # On Windows, use `venv\Scripts\activate`
   ```

3. **Install dependencies:**
   ```sh
   pip install -r requirements.txt
   ```

## Usage

1. **Run the Flask application:**
   ```sh
   python app.py
   ```

2. **Access the application:**
   Open your browser and navigate to `http://localhost:5001`.

## Project Structure

- `app.py`: Main Flask application file.
- `rebuild_model.py`: Script to rebuild and save the TensorFlow model.
- `Data/`: Directory containing gesture data files.
- `App/`: Directory containing application-specific files.

## License

This project is licensed under the MIT License - see the LICENSE file for details. 