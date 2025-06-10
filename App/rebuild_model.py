import tensorflow as tf
import numpy as np

def build_model(input_shape, num_classes):
    """Build a neural network model for gesture classification."""
    model = tf.keras.Sequential([
        tf.keras.Input(shape=(input_shape,)),
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dropout(0.3),
        tf.keras.layers.Dense(64, activation='relu'),
        tf.keras.layers.Dropout(0.2),
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])

    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )

    return model

# Create and save the model
input_shape = 300  # Based on your sequence length (100) * features (3)
num_classes = 2    # Based on your GESTURE_CLASSES ["O", "V"]

model = build_model(input_shape, num_classes)

# Save the model in a format compatible with TensorFlow 2.13.0
model.save("wand_model.h5", save_format="h5")
print("Model saved successfully!") 