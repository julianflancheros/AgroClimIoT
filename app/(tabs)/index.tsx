// import { Image } from 'expo-image';
// import { Platform, StyleSheet } from 'react-native';

// import { HelloWave } from '@/components/hello-wave';
// import ParallaxScrollView from '@/components/parallax-scroll-view';
// import { ThemedText } from '@/components/themed-text';
// import { ThemedView } from '@/components/themed-view';
// import { Link } from 'expo-router';

// export default function HomeScreen() {
//   return (
//     <ParallaxScrollView
//       headerBackgroundColor={{ light: '#A1CEDC', dark: '#1D3D47' }}
//       headerImage={
//         <Image
//           source={require('@/assets/images/partial-react-logo.png')}
//           style={styles.reactLogo}
//         />
//       }>
//       <ThemedView style={styles.titleContainer}>
//         <ThemedText type="title">Welcome!</ThemedText>
//         <HelloWave />
//       </ThemedView>
//       <ThemedView style={styles.stepContainer}>
//         <ThemedText type="subtitle">Step 1: Try it</ThemedText>
//         <ThemedText>
//           Edit <ThemedText type="defaultSemiBold">app/(tabs)/index.tsx</ThemedText> to see changes.
//           Press{' '}
//           <ThemedText type="defaultSemiBold">
//             {Platform.select({
//               ios: 'cmd + d',
//               android: 'cmd + m',
//               web: 'F12',
//             })}
//           </ThemedText>{' '}
//           to open developer tools.
//         </ThemedText>
//       </ThemedView>
//       <ThemedView style={styles.stepContainer}>
//         <Link href="/modal">
//           <Link.Trigger>
//             <ThemedText type="subtitle">Step 2: Explore</ThemedText>
//           </Link.Trigger>
//           <Link.Preview />
//           <Link.Menu>
//             <Link.MenuAction title="Action" icon="cube" onPress={() => alert('Action pressed')} />
//             <Link.MenuAction
//               title="Share"
//               icon="square.and.arrow.up"
//               onPress={() => alert('Share pressed')}
//             />
//             <Link.Menu title="More" icon="ellipsis">
//               <Link.MenuAction
//                 title="Delete"
//                 icon="trash"
//                 destructive
//                 onPress={() => alert('Delete pressed')}
//               />
//             </Link.Menu>
//           </Link.Menu>
//         </Link>

//         <ThemedText>
//           {`Tap the Explore tab to learn more about what's included in this starter app.`}
//         </ThemedText>
//       </ThemedView>
//       <ThemedView style={styles.stepContainer}>
//         <ThemedText type="subtitle">Step 3: Get a fresh start</ThemedText>
//         <ThemedText>
//           {`When you're ready, run `}
//           <ThemedText type="defaultSemiBold">npm run reset-project</ThemedText> to get a fresh{' '}
//           <ThemedText type="defaultSemiBold">app</ThemedText> directory. This will move the current{' '}
//           <ThemedText type="defaultSemiBold">app</ThemedText> to{' '}
//           <ThemedText type="defaultSemiBold">app-example</ThemedText>.
//         </ThemedText>
//       </ThemedView>
//     </ParallaxScrollView>
//   );
// }

// const styles = StyleSheet.create({
//   titleContainer: {
//     flexDirection: 'row',
//     alignItems: 'center',
//     gap: 8,
//   },
//   stepContainer: {
//     gap: 8,
//     marginBottom: 8,
//   },
//   reactLogo: {
//     height: 178,
//     width: 290,
//     bottom: 0,
//     left: 0,
//     position: 'absolute',
//   },
// });


import { onValue, ref } from 'firebase/database';
import React, { useEffect, useState } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import { database } from '../../firebaseConfig';

type SensorData = {
  temp_ambiente?: number;
  hum_suelo?: number;
  presion?: number;
  timestamp?: string;
};

export default function App() {
  const [sensorData, setSensorData] = useState<SensorData | null>(null);

  useEffect(() => {
    // Referencia al nodo específico que creamos en el ESP32
    // Recuerda que en el Arduino pusimos: "sensores/NODO_AGRO_01"
    const starCountRef = ref(database, 'sensores/NODO_AGRO_01');
    
    // Escuchar cambios en tiempo real
    onValue(starCountRef, (snapshot) => {
      const data = snapshot.val();
      setSensorData(data);
    });
  }, []);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>🌱 Monitor Agro-IoT</Text>
      <Text style={styles.subtitle}>Panel de Control en Tiempo Real</Text>

      <ScrollView contentContainerStyle={styles.scroll}>
        {/* Tarjeta de Temperatura Ambiente */}
        <View style={[styles.card, { backgroundColor: '#FFD700' }]}>
          <Text style={styles.cardLabel}>🌡️ Temp. Ambiente</Text>
          <Text style={styles.cardValue}>
            {sensorData ? sensorData.temp_ambiente : '--'} °C
          </Text>
        </View>

        {/* Tarjeta de Humedad Suelo (La más importante) */}
        <View style={[styles.card, { backgroundColor: '#4CAF50' }]}>
          <Text style={styles.cardLabel}>💧 Humedad Suelo</Text>
          <Text style={styles.cardValue}>
            {sensorData ? sensorData.hum_suelo : '--'} %
          </Text>
          <Text style={styles.status}>
            {sensorData && sensorData.hum_suelo !== undefined && sensorData.hum_suelo < 30
              ? "⚠️ RIEGO NECESARIO"
              : "✅ Humedad Óptima"}
          </Text>
        </View>

        {/* Tarjeta de Presión */}
        <View style={[styles.card, { backgroundColor: '#2196F3' }]}>
          <Text style={styles.cardLabel}>⏲️ Presión</Text>
          <Text style={styles.cardValue}>
            {sensorData ? sensorData.presion : '--'} hPa
          </Text>
        </View>
        
        <Text style={styles.footer}>
           Última act: {sensorData ? sensorData.timestamp : 'Esperando datos...'}
        </Text>
      </ScrollView>
    </View>
  );
}

// Estilos (Como CSS pero en JS)
const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f4f4f4',
    paddingTop: 50,
    alignItems: 'center',
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 5,
  },
  subtitle: {
    fontSize: 14,
    color: '#666',
    marginBottom: 20,
  },
  scroll: {
    width: '100%',
    alignItems: 'center',
    paddingBottom: 30,
  },
  card: {
    width: '90%', // Ocupa el 90% del ancho de la pantalla
    padding: 20,
    borderRadius: 15,
    marginBottom: 15,
    shadowColor: "#000",
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 5, // Sombra en Android
  },
  cardLabel: {
    fontSize: 18,
    fontWeight: '600',
    color: '#fff',
    textShadowColor: 'rgba(0, 0, 0, 0.2)',
    textShadowOffset: {width: 1, height: 1},
    textShadowRadius: 2,
  },
  cardValue: {
    fontSize: 32,
    fontWeight: 'bold',
    color: '#fff',
    marginTop: 5,
  },
  status: {
    marginTop: 10,
    fontSize: 16,
    fontWeight: 'bold',
    color: '#fff',
    backgroundColor: 'rgba(0,0,0,0.2)',
    padding: 5,
    borderRadius: 5,
    textAlign: 'center'
  },
  footer: {
    marginTop: 20,
    color: '#888',
    fontSize: 12,
  }
});