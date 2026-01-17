import React, { useState } from 'react';
import { View, Image, Text, StyleSheet } from 'react-native';

import LoginScreen from './LoginScreen';
import ProfileScreen from './ProfileScreen';
import CalorieResultScreen from './CalorieResultScreen';
import RecipesList from './recipesList';
import logo from './assets/logo.png';

export default function App() {
  const [user, setUser] = useState(null);
  const [screen, setScreen] = useState('login');
  const [calorieData, setCalorieData] = useState(null);

  return (
    <View style={{ flex: 1, backgroundColor: '#fff' }}>
      <View style={styles.header}>
        <Image source={logo} style={styles.logo} />
        <Text style={styles.appName}>NutriFit</Text>
      </View>

      {screen === 'login' && (
        <LoginScreen
          onLogin={(userData) => {
            setUser(userData);
            setScreen('profile');
          }}
        />
      )}

      {screen === 'profile' && (
        <ProfileScreen
          user={user}
          onLogout={() => {
            setUser(null);
            setScreen('login');
          }}
          onSaveSuccess={(result) => {
            setCalorieData(result);
            setScreen('calories');
          }}
        />
      )}

      {screen === 'calories' && calorieData && (
        <CalorieResultScreen
          data={calorieData}
          onBack={() => setScreen('profile')}
          onMealoption={() => setScreen('recipes')}
        />
      )}
      {screen === 'recipes' && (
        <RecipesList
          data={calorieData}
          onBack={() => setScreen('profile')}
        />
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingTop: 40,
    paddingBottom: 20,
  },
  logo: {
    width: 55,
    height: 55,
    resizeMode: 'contain',
    marginRight: 10,
  },
  appName: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#28B572',
  },
});
