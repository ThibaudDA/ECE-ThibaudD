# Importation des bibliothèques nécessaires pour l’apprentissage profond
import os
from pathlib import Path
import torch
import torch.nn as nn
from torch.utils.data import DataLoader
from torchvision import datasets, transforms, models


def main():

    # Chemins vers les dossiers d’entraînement et de validation
    train_dir = "C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Split\\Training"
    val_dir = "C:\\Users\\thiba\\OneDrive\\Bureau\\AI_Project\\Split\\Validation"

    # Hyperparamètres
    batch_size = 16        # Nombre d’images par batch
    num_epochs = 20        # Nombre total d’époques d’entraînement
    lr = 1e-4              # Taux d’apprentissage
    device = "cuda" if torch.cuda.is_available() else "cpu"  # Sélection GPU/CPU
    

    # Transformations appliquées aux images d’entraînement (avec augmentation)
    train_tf = transforms.Compose([
        transforms.Resize((224, 224)),            # Redimensionnement
        transforms.RandomHorizontalFlip(),        # Flip horizontal aléatoire
        transforms.RandomRotation(10),            # Rotation aléatoire ±10°
        transforms.ToTensor(),                   # Conversion en tenseur PyTorch
        transforms.Normalize([0.485, 0.456, 0.406],   # Normalisation ImageNet
                             [0.229, 0.224, 0.225]),
    ])
    
    # Transformations appliquées au set de validation (pas d’augmentation)
    val_tf = transforms.Compose([
        transforms.Resize((224, 224)),            # Même résolution que l’entraînement
        transforms.ToTensor(),                   # Tensor
        transforms.Normalize([0.485, 0.456, 0.406],
                             [0.229, 0.224, 0.225]),
    ])
    

    # Chargement des datasets depuis la structure de dossiers
    train_ds = datasets.ImageFolder(train_dir, transform=train_tf)
    val_ds = datasets.ImageFolder(val_dir, transform=val_tf)
    
    # Création des DataLoaders pour charger les images par batch
    train_loader = DataLoader(train_ds, batch_size=batch_size, shuffle=True)   # On mélange les images
    val_loader = DataLoader(val_ds, batch_size=batch_size, shuffle=False)      # Pas de shuffle pour la validation
    
    num_classes = len(train_ds.classes)   # Nombre de classes détectées automatiquement


    # Affichage des classes pour vérification
    classes_str = ", ".join(train_ds.classes)
    print(f"Number of classes : {num_classes}", flush=True)
    print(f"Classes : [{classes_str}]\n", flush=True)

    # Chargement d’un modèle ConvNeXt Base préentraîné sur ImageNet
    weights = models.ConvNeXt_Base_Weights.IMAGENET1K_V1
    model = models.convnext_base(weights=weights)
    
    # Remplacement de la couche finale du classifieur pour correspondre au nombre de classes
    in_features = model.classifier[2].in_features
    model.classifier[2] = nn.Linear(in_features, num_classes)
    
    # Envoi du modèle sur GPU ou CPU
    model = model.to(device)
    
    # Définition de la fonction de perte et de l’optimiseur
    criterion = nn.CrossEntropyLoss()                       # Perte adaptée au multi-classe
    optimizer = torch.optim.Adam(model.parameters(), lr=lr) # Optimiseur Adam

    best_acc = 0.0                # Meilleure précision obtenue
    patience_count = 0            # Compteur pour un éventuel early stopping (non utilisé ici)
    best_path = "convnext_base_custom.pt"   # Chemin où sauvegarder le meilleur modèle
    
    # Boucle d’entraînement principale
    for epoch in range(1, num_epochs + 1):
        print(f"\n--- Epoch {epoch}/{num_epochs} ---", flush=True)
    
        # Phase d’entraînement
        model.train()             # Passage en mode entraînement
        train_loss = 0.0
        correct = 0
        total = 0
    
        for images, labels in train_loader:
            images, labels = images.to(device), labels.to(device)
    
            # Forward + Backward + Optimisation
            optimizer.zero_grad()        # Réinitialisation des gradients
            outputs = model(images)      # Prédiction
            loss = criterion(outputs, labels)  # Calcul de la perte
            loss.backward()              # Rétropropagation
            optimizer.step()             # Mise à jour des poids
    
            # Calcul des métriques d'entraînement
            train_loss += loss.item() * images.size(0)
            _, pred = torch.max(outputs, 1)
            correct += (pred == labels).sum().item()
            total += labels.size(0)
    
        train_acc = correct / total
        train_loss /= total
    
        # Phase de validation
        model.eval()             # Mode évaluation (désactive dropout, batchnorm training)
        val_loss = 0.0
        correct = 0
        total = 0
    
        with torch.no_grad():    # Pas de calcul de gradient
            for images, labels in val_loader:
                images, labels = images.to(device), labels.to(device)
    
                outputs = model(images)          # Forward seul
                loss = criterion(outputs, labels)
    
                # Suivi des métriques
                val_loss += loss.item() * images.size(0)
                _, pred = torch.max(outputs, 1)
                correct += (pred == labels).sum().item()
                total += labels.size(0)
    
        val_acc = correct / total
        val_loss /= total
    
        # Affichage des statistiques
        print(f"Train loss: {train_loss:.4f} | acc: {train_acc:.4f}", flush=True)
        print(f"Val   loss: {val_loss:.4f} | acc: {val_acc:.4f}", flush=True)
    
        # Sauvegarde du meilleur modèle (basé sur la précision validation)
        if val_acc > best_acc:
            best_acc = val_acc
            patience_count = 0
            torch.save({
                "model": model.state_dict(),
                "classes": train_ds.classes,
                "model_name": "convnext_base",
                "epoch": epoch,
                "val_acc": val_acc
            }, best_path)
            print(f"New best model saved in {best_path}", flush=True)
    
    # Fin de l’entraînement
    print("\nTraining session finished.", flush=True)
    print(f"Improved validation accuracy : {best_acc:.4f}", flush=True)
    

# Point d’entrée du script
if __name__ == "__main__":
    main()
