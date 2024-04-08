# Install and load required libraries
install.packages(c("randomForest", "ggplot2", "caret"))
library(randomForest)
library(ggplot2)
library(caret)

# Load the Wine data set
wine <- read.csv("https://archive.ics.uci.edu/ml/machine-learning-databases/wine/wine.data", header = FALSE)
colnames(wine) <- c("Class", "Alcohol", "MalicAcid", "Ash", "AlcalinityOfAsh", "Magnesium", "TotalPhenols", "Flavanoids", "NonflavanoidPhenols", "Proanthocyanins", "ColorIntensity", "Hue", "OD280_OD315", "Proline")

# Convert the 'Class' variable to a factor
wine$Class <- as.factor(wine$Class)

# Data Representation
# Scatter plot for Color Intensity vs. Hue colored by Class
ggplot(wine, aes(x = ColorIntensity, y = Hue, color = Class)) +
  geom_point() +
  labs(title = "Scatter Plot of Color Intensity vs. Hue", x = "Color Intensity", y = "Hue") +
  theme_minimal()

# Split the data into training and testing sets
set.seed(123)
train_indices <- sample(1:nrow(wine), 0.7 * nrow(wine))
train_data <- wine[train_indices, ]
test_data <- wine[-train_indices, ]

# Train a Random Forest model
rf_model <- randomForest(Class ~ ., data = train_data, ntree = 100)

# Make predictions on the test set
predictions <- predict(rf_model, newdata = test_data)

# Confusion matrix
conf_matrix <- table(predictions, test_data$Class)
print(conf_matrix)

# Accuracy
accuracy <- sum(diag(conf_matrix)) / sum(conf_matrix)
print(paste("Accuracy:", accuracy))

# Feature importance plot
varImpPlot(rf_model)

