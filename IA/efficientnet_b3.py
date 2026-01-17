# Import necessary libraries for deep learning and image processing
import torch
import torch.nn as nn
import torch.optim as optim
from torchvision import datasets, transforms, models
from torch.utils.data import DataLoader
from pathlib import Path
from tqdm import tqdm
import time

# Define paths to training and validation datasets
train_dir = "C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Split\\Training"
val_dir = "C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Split\\Validation"

# Hyperparameters configuration
batch_size = 6          # Number of images per batch
epochs = 60             # Maximum number of training epochs
lr = 8e-5               # Learning rate
weight_decay = 8e-4     # L2 regularization parameter
seed = 42               # Random seed for reproducibility

# Set random seed for reproducibility
torch.manual_seed(seed)
# Use GPU if available, otherwise CPU
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print("Device:", device)

# ImageNet normalization values (standard for pretrained models)
mean = [0.485, 0.456, 0.406]
std  = [0.229, 0.224, 0.225]

# Data augmentation for training set to improve model generalization
train_tf = transforms.Compose([
    transforms.Resize((256, 256)),                              # Resize to 256x256
    transforms.RandomResizedCrop(224, scale=(0.85, 1.0)),      # Random crop to 224x224
    transforms.RandomHorizontalFlip(p=0.5),                     # Random horizontal flip
    transforms.RandomRotation(10),                              # Random rotation ±10 degrees
    transforms.ColorJitter(0.25, 0.25, 0.2, 0.08),             # Random color variations
    transforms.RandomPerspective(0.12, p=0.2),                  # Random perspective transform
    transforms.ToTensor(),                                      # Convert to tensor
    transforms.Normalize(mean, std),                            # Normalize with ImageNet stats
])

# Simple transformations for validation set (no augmentation)
val_tf = transforms.Compose([
    transforms.Resize((224, 224)),      # Resize to 224x224
    transforms.ToTensor(),              # Convert to tensor
    transforms.Normalize(mean, std),    # Normalize with ImageNet stats
])

# Filter function to exclude hidden or system files
def valid_img(path):
    name = Path(path).name
    # Skip hidden files and macOS resource forks
    if name.startswith(".") or name.startswith("._"):
        return False
    # Accept only JPG/JPEG files
    return name.lower().endswith((".jpg", ".jpeg"))

# Load datasets from folder structure (each subfolder = one class)
train_ds = datasets.ImageFolder(train_dir, transform=train_tf, is_valid_file=valid_img)
val_ds   = datasets.ImageFolder(val_dir, transform=val_tf, is_valid_file=valid_img)
num_classes = len(train_ds.classes)

# Create data loaders for batch processing
train_loader = DataLoader(train_ds, batch_size=batch_size, shuffle=True)   # Shuffle training data
val_loader   = DataLoader(val_ds, batch_size=batch_size, shuffle=False)    # Don't shuffle validation

print("Number of classes :", num_classes)
print("Train images :", len(train_ds))
print("Val images   :", len(val_ds))


def create_efficientnet_b3(num_classes):
    """Create EfficientNet-B3 model with custom classifier head."""
    # Load pretrained weights from ImageNet
    weights = models.EfficientNet_B3_Weights.IMAGENET1K_V1
    model = models.efficientnet_b3(weights=weights)

    # Replace the classifier with a custom multi-layer head
    in_features = model.classifier[1].in_features
    model.classifier = nn.Sequential(
        nn.Dropout(0.45),                       # Dropout for regularization
        nn.Linear(in_features, 640),            # First dense layer
        nn.ReLU(),                              # Activation function
        nn.Dropout(0.4),                        # Dropout
        nn.Linear(640, 512),                    # Second dense layer
        nn.ReLU(),                              # Activation function
        nn.Dropout(0.35),                       # Dropout
        nn.Linear(512, num_classes)             # Output layer
    )

    return model.to(device)


def run_epoch(model, loader, criterion, optimizer=None, train=False):
    """Run one epoch of training or validation."""
    # Set model mode (training or evaluation)
    model.train(train)
    total, correct, loss_sum = 0, 0, 0.0

    # Progress bar for visual feedback
    loop = tqdm(loader, desc="Train" if train else "Val", leave=False)
    for imgs, labels in loop:
        # Move data to device (GPU/CPU)
        imgs, labels = imgs.to(device), labels.to(device)

        if train:
            optimizer.zero_grad()  # Reset gradients

        # Enable/disable gradient computation
        with torch.set_grad_enabled(train):
            logits = model(imgs)            # Forward pass
            loss = criterion(logits, labels) # Compute loss

            if train:
                loss.backward()      # Backward pass
                optimizer.step()     # Update weights

        # Calculate accuracy
        preds = logits.argmax(1)
        bsz = labels.size(0)
        total += bsz
        correct += (preds == labels).sum().item()
        loss_sum += loss.item() * bsz

        # Update progress bar
        loop.set_postfix(loss=f"{loss.item():.4f}", acc=f"{correct/total:.4f}")

    return loss_sum / total, correct / total


def train_model():
    """Main training loop with early stopping and learning rate scheduling."""
    # Initialize model
    model = create_efficientnet_b3(num_classes)

    # Loss function with label smoothing to prevent overconfidence
    criterion = nn.CrossEntropyLoss(label_smoothing=0.15)
    # AdamW optimizer (Adam with weight decay)
    optimizer = optim.AdamW(model.parameters(), lr=lr, weight_decay=weight_decay)

    # Learning rate scheduler: reduce LR when validation accuracy plateaus
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(
        optimizer, mode='max', factor=0.45, patience=4, min_lr=5e-7
    )

    # Early stopping configuration
    patience = 15      # Stop if no improvement after 15 epochs
    patience_count = 0

    best_acc = 0.0
    best_path = "efficientnet_b3.pt"

    start = time.time()     # Track training time

    # Training loop
    for epoch in range(1, epochs + 1):
        # Run training epoch
        tr_loss, tr_acc = run_epoch(model, train_loader, criterion, optimizer, train=True)
        # Run validation epoch
        va_loss, va_acc = run_epoch(model, val_loader, criterion, train=False)

        print(f"Epoch {epoch:02d}/{epochs} | "
              f"Train acc={tr_acc:.4f} | Val acc={va_acc:.4f}")

        # Adjust learning rate based on validation accuracy
        scheduler.step(va_acc)

        # Save model if validation accuracy improved
        if va_acc > best_acc:
            best_acc = va_acc
            patience_count = 0
            # Save model checkpoint with metadata
            torch.save({
                "model": model.state_dict(),
                "classes": train_ds.classes,
                "model_name": "efficientnet_b3",
                "epoch": epoch,
                "val_acc": va_acc
            }, best_path)
            print(f" New best saved model (acc={va_acc:.4f})")
        else:
            # Increment patience counter if no improvement
            patience_count += 1
            if patience_count >= patience:
                print("\n Early Stopping: more progress")
                break

    # Print training summary
    total_min = (time.time() - start) / 60
    print(f"\nCompleted in {total_min:.1f} min")
    print(f"Improved validation accuracy : {best_acc:.4f}")
    print(f"Model saved in : {best_path}")

    return best_path


# Entry point start training when script is run directly
if __name__ == "__main__":
    train_model()
