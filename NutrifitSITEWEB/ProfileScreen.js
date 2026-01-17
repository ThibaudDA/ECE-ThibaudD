import React, { useState, useEffect } from "react";
import {
  View,
  Text,
  TextInput,
  Button,
  Alert,
  ScrollView,
  TouchableOpacity,
  StyleSheet,
} from "react-native";

import { createClient } from "@supabase/supabase-js";

const SUPABASE_URL = "https://gclpgggqbmbcaenzidgh.supabase.co";
const SUPABASE_KEY =
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImdjbHBnZ2dxYm1iY2FlbnppZGdoIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjI3ODI5MjEsImV4cCI6MjA3ODM1ODkyMX0.dIsT_dnWNPFqNpB5C4cY5ZSRetzL1k_B3Fu81XzLQeY";

const supabase = createClient(SUPABASE_URL, SUPABASE_KEY);

export default function ProfileScreen({ user, onLogout, onSaveSuccess }) {
  const [form, setForm] = useState({
    name: "",
    age: "",
    height: "",
    weight: "",
    gender: "",
    activity_level: "",
  });

  const [showActivityMenu, setShowActivityMenu] = useState(false);

  useEffect(() => {
    setForm({
      name: user.name || "",
      age: user.age?.toString() || "",
      height: user.height?.toString() || "",
      weight: user.weight?.toString() || "",
      gender: user.gender || "",
      activity_level: user.activity_level || "",
    });
  }, [user]);

  const handleSave = async () => {
    if (
      !form.weight ||
      !form.height ||
      !form.age ||
      !form.gender ||
      !form.activity_level
    ) {
      Alert.alert("Error", "Please fill all fields before saving.");
      return;
    }

    const { error } = await supabase
      .from("user")
      .update(form)
      .eq("id", user.id);

    if (error) {
      Alert.alert("Error", error.message);
      return;
    }

    const weight = parseFloat(form.weight);
    const heightCm = parseFloat(form.height);
    const age = parseFloat(form.age);

    let BMR = 0;

    if (form.gender === "Man") {
      BMR = 10 * weight + 6.25 * heightCm - 5 * age + 5;
    } else {
      BMR = 10 * weight + 6.25 * heightCm - 5 * age - 161;
    }

    const activityFactors = {
      Sedentary: 1.17,
      "Lightly Active": 1.27,
      "Moderately Active": 1.37,
      "Very Active": 1.52,
      "Extra Active": 1.65,
    };

    const factor = activityFactors[form.activity_level] || 1.2;

    const TDEE = BMR * factor;

    const heightMeters = heightCm / 100;
    const BMI = weight / (heightMeters * heightMeters);

    let targetCalories = TDEE;
    let advice = "";

    if (BMI < 18.5) {
      targetCalories = TDEE + 300;
      advice = "You are underweight. Increasing calories is recommended.";
    } else if (BMI <= 24.9) {
      targetCalories = TDEE;
      advice = "You are at a healthy weight. Maintain your intake.";
    } else {
      targetCalories = TDEE - 300;
      advice = "You are overweight. A calorie deficit is recommended.";
    }

    onSaveSuccess({
      BMI,
      BMR,
      TDEE,
      targetCalories,
      advice,
    });
  };

  const activityOptions = [
    "Sedentary",
    "Lightly Active",
    "Moderately Active",
    "Very Active",
    "Extra Active",
  ];

  return (
    <ScrollView contentContainerStyle={styles.container}>
      <Text style={styles.title}>Profile</Text>

      <Text style={styles.label}>Name :</Text>
      <TextInput
        style={styles.input}
        placeholder="Your name"
        value={form.name}
        onChangeText={(v) => setForm({ ...form, name: v })}
      />

      <Text style={styles.label}>Age :</Text>
      <TextInput
        style={styles.input}
        placeholder="Your age"
        keyboardType="numeric"
        value={form.age}
        onChangeText={(v) => setForm({ ...form, age: v })}
      />

      <Text style={styles.label}>Height (cm) :</Text>
      <TextInput
        style={styles.input}
        placeholder="Your height"
        keyboardType="numeric"
        value={form.height}
        onChangeText={(v) => setForm({ ...form, height: v })}
      />

      <Text style={styles.label}>Weight (kg) :</Text>
      <TextInput
        style={styles.input}
        placeholder="Your weight"
        keyboardType="numeric"
        value={form.weight}
        onChangeText={(v) => setForm({ ...form, weight: v })}
      />

      <Text style={styles.label}>Gender :</Text>

      <View style={styles.genderContainer}>
        <TouchableOpacity
          style={[
            styles.genderOption,
            form.gender === "Man" && styles.genderSelected,
          ]}
          onPress={() => setForm({ ...form, gender: "Man" })}
        >
          <Text
            style={[
              styles.genderText,
              form.gender === "Man" && styles.genderTextSelected,
            ]}
          >
            Man
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[
            styles.genderOption,
            form.gender === "Woman" && styles.genderSelected,
          ]}
          onPress={() => setForm({ ...form, gender: "Woman" })}
        >
          <Text
            style={[
              styles.genderText,
              form.gender === "Woman" && styles.genderTextSelected,
            ]}
          >
            Woman
          </Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.label}>Activity level:</Text>

      <TouchableOpacity
        onPress={() => setShowActivityMenu(!showActivityMenu)}
        style={styles.select}
      >
        <Text style={styles.selectText}>
          {form.activity_level || "Select your activity level"}
        </Text>
      </TouchableOpacity>

      {showActivityMenu &&
        activityOptions.map((opt) => (
          <TouchableOpacity
            key={opt}
            onPress={() => {
              setForm({ ...form, activity_level: opt });
              setShowActivityMenu(false);
            }}
            style={styles.option}
          >
            <Text style={styles.optionText}>{opt}</Text>
          </TouchableOpacity>
        ))}

      <View style={styles.buttonWrapper}>
        <Button title="Save" onPress={handleSave} color="#28B572" />
      </View>

      <View style={styles.logoutWrapper}>
        <Button title="Disconnect" color="red" onPress={onLogout} />
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: "center",
    backgroundColor: "#FFFFFF",
    paddingBottom: 30,
  },
  title: {
    fontSize: 26,
    fontWeight: "bold",
    color: "#28B572",
    marginVertical: 20,
  },
  label: {
    width: "80%",
    fontSize: 16,
    marginBottom: 5,
  },
  input: {
    width: "80%",
    borderWidth: 3,
    borderColor: "#000",
    padding: 10,
    borderRadius: 8,
    fontSize: 16,
    marginBottom: 15,
  },
  genderContainer: {
    flexDirection: "row",
    justifyContent: "space-around",
    width: "80%",
    marginBottom: 15,
  },
  genderOption: {
    borderWidth: 3,
    borderColor: "#000",
    paddingVertical: 10,
    paddingHorizontal: 30,
    borderRadius: 8,
  },
  genderSelected: {
    backgroundColor: "#28B572",
    borderColor: "#28B572",
  },
  genderText: {
    fontSize: 16,
  },
  genderTextSelected: {
    color: "white",
    fontWeight: "bold",
  },
  select: {
    width: "80%",
    borderWidth: 3,
    borderColor: "#000",
    padding: 12,
    borderRadius: 8,
    marginBottom: 15,
  },
  selectText: {
    fontSize: 16,
  },
  option: {
    width: "80%",
    backgroundColor: "#28B572",
    padding: 10,
    marginBottom: 8,
    borderRadius: 8,
  },
  optionText: {
    color: "white",
    fontSize: 16,
  },
  buttonWrapper: {
    width: "80%",
    marginTop: 10,
  },
  logoutWrapper: {
    width: "80%",
    marginTop: 20,
  },
});
