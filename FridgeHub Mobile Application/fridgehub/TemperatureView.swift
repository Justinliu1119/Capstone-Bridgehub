// TemperatureView.swift
//           EE 459 Embeded System Laboratory
//                   Spring 2024
//   Created by Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
//                   05/06/2024
//

import Foundation
import SwiftUI
import FirebaseDatabase

struct TemperatureView: View {
    @State private var temperature: String = "Loading..."
    private var databaseRef = Database.database().reference()

    var body: some View {
        VStack(spacing: 20) {
            Text("Current Temperature")
                .font(.title2)
            Text(temperature)
                .font(.largeTitle)
            Button("Refresh") {
                fetchTemperature()
            }
        }
        .padding()
        .onAppear {
            setupTemperatureObserver()
        }
    }

    private func setupTemperatureObserver() {
        // Assuming 'temperature' is directly under the root, adjust the path as necessary
        databaseRef.child("temperature").observe(.value) { snapshot in
            if let tempValue = snapshot.value as? Double {
                // Format the temperature as needed, here we append "°C"
                self.temperature = "\(tempValue)°F"
            } else {
                self.temperature = "Unable to retrieve temperature"
            }
        }
    }

    private func fetchTemperature() {
        // Manual refresh to fetch the latest temperature
        databaseRef.child("temperature").getData { error, snapshot in
            if let error = error {
                print("Error getting data \(error)")
                self.temperature = "Fetch error"
            } else if let tempValue = snapshot?.value as? Double {
                self.temperature = "\(tempValue)°F"
            } else {
                self.temperature = "No temperature data"
            }
        }
    }
}

struct TemperatureView_Previews: PreviewProvider {
    static var previews: some View {
        TemperatureView()
    }
}
