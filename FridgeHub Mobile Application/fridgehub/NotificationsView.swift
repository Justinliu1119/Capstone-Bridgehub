//
// NotificationsView.swift
//           EE 459 Embeded System Laboratory
//                   Spring 2024
//   Created by Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
//                   05/06/2024
//


import Foundation
import SwiftUI
import FirebaseDatabase
struct NotificationsView: View {
    @ObservedObject var viewModel: GroceryViewModel  // This should be passed in during initialization
    @State private var temperature: Double? = nil  // This is private and correctly managed within the view

    private var databaseRef = Database.database().reference()  // This remains private as it's internal configuration
    public init(viewModel: GroceryViewModel) {
            _viewModel = ObservedObject(initialValue: viewModel)
        }
    var body: some View {
        List {
            upcomingExpirationsSection
            temperatureAlertSection
        }
        .navigationTitle("Notifications")
        .onAppear(perform: loadTemperature)
    }

    private func loadTemperature() {
        databaseRef.child("temperature").observeSingleEvent(of: .value) { snapshot in
            if let tempValue = snapshot.value as? Double {
                self.temperature = tempValue
            } else {
                self.temperature = nil  // Handle the case where temperature cannot be retrieved
            }
        }
    }

    private var upcomingExpirationsSection: some View {
        Section(header: Text("Upcoming Expirations")) {
            ForEach(viewModel.groceries.filter { $0.expirationDate < Date().addingTimeInterval(86400) && $0.quantity > 0 }) { item in
                Text("\(item.name) expires soon!")
            }
        }
    }

    private var temperatureAlertSection: some View {
        Section(header: Text("Temperature Alerts")) {
            if let temp = temperature, temp > 80 {
                Text("Warning: High temperature (\(String(format: "%.1f", temp))°F)")
            } else {
                Text("Temperature data not available")
            }
        }
    }
    
}
struct NotificationsView_Previews: PreviewProvider {
    static var previews: some View {
        // Assuming GroceryViewModel can be initialized with default or dummy data
        NotificationsView(viewModel: GroceryViewModel())
            .previewLayout(.sizeThatFits)
            .padding()
    }
}
