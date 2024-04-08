# Load the Chick Weight data set
data(ChickWeight)
head(ChickWeight)

# Create a binary response variable
ChickWeight$AboveThreshold <- ifelse(ChickWeight$weight > 200, 1, 0)

# Convert 'Diet' to a factor
ChickWeight$Diet <- as.factor(ChickWeight$Diet)

# Perform logistic regression
logistic_model <- glm(AboveThreshold ~ Time + Diet, data = ChickWeight, family = "binomial")

# Display summary of the logistic regression
summary(logistic_model)

# Visualize the results
# Scatter plot of the original data
plot(ChickWeight$Time, ChickWeight$AboveThreshold, col = ChickWeight$AboveThreshold + 1, pch = 19, xlab = "Time", ylab = "Above Threshold")

# Add logistic regression curve
x_values <- seq(min(ChickWeight$Time), max(ChickWeight$Time), length.out = 100)
y_probabilities <- predict(logistic_model, newdata = data.frame(Time = x_values, Diet = factor(rep(1, length(x_values)), levels = levels(ChickWeight$Diet))), type = "response")
lines(x_values, y_probabilities, col = "blue", lwd = 2)
