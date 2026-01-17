# student_infer.py
# NO IMPORTS ALLOWED.
# Uses injected torch, nn, models from marker.py

# Ensemble model that combines multiple models with weighted averaging
class WeightedEnsembleModel:

    def __init__(self, models_list, weights):
        self.models = models_list
        self.weights = weights

    def eval(self):
        for m in self.models:
            m.eval()
        return self

    def train(self):
        for m in self.models:
            m.train()
        return self

    def to(self, device):
        for m in self.models:
            m.to(device)
        return self

    def __call__(self, x):
        # Combine predictions from all models using weights
        weighted_logits = None
        for model, w in zip(self.models, self.weights):
            out = model(x) * w
            if weighted_logits is None:
                weighted_logits = out
            else:
                weighted_logits += out
        return weighted_logits


# Load ConvNeXt model from checkpoint
def load_convnext(torch, nn, models):
    ckpt = torch.load("convnext_base_custom.pt", map_location="cpu")
    classes = [str(c) for c in ckpt["classes"]]
    num_classes = len(classes)

    # Rebuild model architecture
    model = models.convnext_base(weights=None)
    in_features = model.classifier[2].in_features
    model.classifier[2] = nn.Linear(in_features, num_classes)

    # Load trained weights
    model.load_state_dict(ckpt["model"], strict=True)
    model.eval()
    return model, classes


# Load EfficientNet model from checkpoint
def load_efficientnet(torch, nn, models):
    ckpt = torch.load("efficientnet_b3.pt", map_location="cpu")
    classes = [str(c) for c in ckpt["classes"]]
    num_classes = len(classes)

    # Rebuild model architecture
    model = models.efficientnet_b3(weights=None)
    in_features = model.classifier[1].in_features

    model.classifier = nn.Sequential(
        nn.Dropout(0.45),
        nn.Linear(in_features, 640),
        nn.ReLU(),
        nn.Dropout(0.4),
        nn.Linear(640, 512),
        nn.ReLU(),
        nn.Dropout(0.35),
        nn.Linear(512, num_classes)
    )

    # Load trained weights
    model.load_state_dict(ckpt["model"], strict=True)
    model.eval()
    return model, classes



# Build ensemble model from saved checkpoints
def build_model(torch, nn, models, classes):

    models_list = []
    classes_ref = None

    # Load ConvNeXt model (required)
    if torch.os.path.exists("convnext_base_custom.pt"):
        convnext_model, classes_conv = load_convnext(torch, nn, models)
        models_list.append(convnext_model)
        classes_ref = classes_conv
    else:
        raise FileNotFoundError("The ConvNeXt file 'convnext_base_custom.pt' is required.")

    # Load EfficientNet model (required)
    if torch.os.path.exists("efficientnet_b3.pt"):
        eff_model, classes_eff = load_efficientnet(torch, nn, models)

        if classes_eff != classes_ref:
            raise ValueError("The classes differ between ConvNeXt and EfficientNet!")

        models_list.append(eff_model)
    else:
        raise FileNotFoundError("The EfficientNet file 'efficientnet_b3.pt' is required.")

    # Create weighted ensemble (ConvNeXt=70%, EfficientNet=30%)
    weights = [0.7, 0.3]

    ensemble = WeightedEnsembleModel(models_list, weights)
    ensemble.eval()

    return ensemble, classes_ref


# Predict class for a single image
def predict(model, image, preprocess, torch):

    # Ensure RGB format
    if image.mode != "RGB":
        image = image.convert("RGB")

    # Preprocess and add batch dimension
    x = preprocess(image).unsqueeze(0)

    # Run inference
    with torch.inference_mode():
        logits = model(x)
        return int(logits.argmax(1).item())
