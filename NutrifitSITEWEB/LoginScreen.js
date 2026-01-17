import React, { useState, useEffect } from "react";
import { View, Text, TextInput, Button, Alert } from "react-native";
import { createClient } from "@supabase/supabase-js";

const SUPABASE_URL = "https://gclpgggqbmbcaenzidgh.supabase.co";
const SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImdjbHBnZ2dxYm1iY2FlbnppZGdoIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjI3ODI5MjEsImV4cCI6MjA3ODM1ODkyMX0.dIsT_dnWNPFqNpB5C4cY5ZSRetzL1k_B3Fu81XzLQeY";

const supabase = createClient(SUPABASE_URL, SUPABASE_KEY);

export default function LoginScreen({ onLogin }) {
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [isRegister, setIsRegister] = useState(false);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    const checkConnection = async () => {
      const { error } = await supabase.from("user").select("id").limit(1);
      if (error) {
        console.error("Database connection error :", error.message);
        Alert.alert("error", "couldn't connect to database");
      } else {
        console.log("Connexion do database successful");
        setConnected(true);
      }
    };
    checkConnection();
  }, []);

  const handleAuth = async () => {
    if (!email || !password) {
      Alert.alert("error", "You need to complete all the fields");
      return;
    }

    if (isRegister) {
      //create account in your database
      const { data, error } = await supabase
        .from("user")
        .insert([{ email, password }])
        .select("*")
        .single();

      if (error) {
        Alert.alert("error", error.message);
        return;
      }

      const { error: signupError } = await supabase.auth.signUp({
        email,
        password,
      });

      // Log CMD pour savoir si l’email est bien envoyé
      console.log(
        "Email confirmation response:",
        signupError ? signupError : "Email sent successfully"
      );

      if (signupError) {
        Alert.alert(
          "Warning",
          "Account created but email confirmation could not be sent: " +
            signupError.message
        );
      } else {
        Alert.alert(
          "Success",
          "Account created! A confirmation email has been sent."
        );
      }


      onLogin(data);

    } else {
      //connection
      const { data, error } = await supabase
        .from("user")
        .select("*")
        .eq("email", email)
        .single();

      if (error || !data) {
        Alert.alert("error", "email address is not registered");
      } else if (data.password !== password) {
        Alert.alert("error", "incorrect password");
      } else {
        Alert.alert("welcome", "successfully connected !");
        onLogin(data);
      }
    }
  };

  return (
    <View style={{ padding: 20, justifyContent: "center", flex: 1 }}>
      <Text
        style={{
          fontSize: 24,
          fontWeight: "bold",
          marginBottom: 10,
          textAlign: "center",
        }}
      >
        {isRegister ? "Create an account" : "Connection"}
      </Text>

      {/*for any database connection error*/}
      <Text
        style={{
          textAlign: "center",
          marginBottom: 15,
          color: connected ? "green" : "red",
        }}
      >
        {connected ? "database connected" : "connecting to database..."}
      </Text>

      <TextInput
        placeholder="Email"
        value={email}
        onChangeText={setEmail}
        style={{
          borderWidth: 1,
          borderColor: "#ccc",
          borderRadius: 8,
          padding: 10,
          marginBottom: 10,
        }}
      />

      <TextInput
        placeholder="Password"
        value={password}
        onChangeText={setPassword}
        secureTextEntry
        style={{
          borderWidth: 1,
          borderColor: "#ccc",
          borderRadius: 8,
          padding: 10,
          marginBottom: 20,
        }}
      />

      <Button
        title={isRegister ? "Create an account" : "Connection"}
        onPress={handleAuth}
      />

      <Text
        style={{ textAlign: "center", marginTop: 15, color: "blue" }}
        onPress={() => setIsRegister(!isRegister)}
      >
        {isRegister ? "Already have an account ?" : "Create an account"}
      </Text>
    </View>
  );
}
