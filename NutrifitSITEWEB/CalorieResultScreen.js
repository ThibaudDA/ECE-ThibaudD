import React from "react";
import { View, Text, Button, StyleSheet } from "react-native";

export default function CalorieResultScreen({ data, onBack, onMealoption }) {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Your Daily Calorie Needs</Text>

      <Text style={styles.info}>BMI: {data.BMI.toFixed(1)}</Text>
      <Text style={styles.info}>BMR: {Math.round(data.BMR)} kcal</Text>
      <Text style={styles.info}>TDEE: {Math.round(data.TDEE)} kcal</Text>

      <Text style={styles.result}>Recommended Daily Calories:</Text>

      <Text style={styles.calories}>
        {Math.round(data.targetCalories)} kcal / day
      </Text>

      <Text style={styles.advice}>{data.advice}</Text>


      <Button title="Back to Profile" onPress={onBack} color="#28B572" />

      <Button title="Meal options" onPress={onMealoption} color="#28B572" />

    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: "center",
    alignItems: "center",
    padding: 20,
  },
  title: {
    fontSize: 26,
    fontWeight: "bold",
    color: "#28B572",
    marginBottom: 20,
  },
  info: {
    fontSize: 18,
    marginBottom: 5,
  },
  result: {
    marginTop: 20,
    fontSize: 20,
    fontWeight: "bold",
  },
  calories: {
    fontSize: 28,
    fontWeight: "bold",
    color: "#ff7f00",
    marginVertical: 10,
  },
  advice: {
    fontSize: 18,
    textAlign: "center",
    marginVertical: 20,
    color: "#555",
  },
});
