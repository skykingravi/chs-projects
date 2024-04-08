# Load the mtcars data set
data(mtcars)
head(mtcars)

# Perform linear regression
lm_model <- lm(mpg ~ wt, data = mtcars)

# Display summary of the linear regression
summary(lm_model)

# Visualize the results
plot(mtcars$wt, mtcars$mpg, main = "Linear Regression", xlab = "Weight", ylab = "Miles Per Gallon", pch = 19, col = "blue")
abline(lm_model, col = "red", lw = 2)

