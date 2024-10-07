//
//  GroceryItemDetailView.swift
// 
//           EE 459 Embeded System Laboratory
//                   Spring 2024
//  Created by Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
//                   05/06/2024
//

import Foundation
import SwiftUI

struct GroceryItemDetailView: View {
    @Binding var item: GroceryItem
    
    var body: some View {
        Form {
            Section(header: Text("Details")) {
                Text("Name: \(item.name)")
                Text("Barcode: \(item.barcode)")
                Text("Category: \(item.category)")
                Text("Quantity: \(item.quantity)")
                Text("Location: \(item.location == .fridge ? "Fridge" : "Freezer")")
                Text("Expiration Date: \(formattedDate(item.expirationDate))")
            }
            
            Section {
                Button(action: {
                    // Add edit action here
                }) {
                    Text("Edit")
                }
            }
        }
        .navigationTitle("Item Details")
    }
    
    private func formattedDate(_ date: Date) -> String {
        let formatter = DateFormatter()
        formatter.dateStyle = .medium
        formatter.timeStyle = .none
        return formatter.string(from: date)
    }
}

struct GroceryItemDetailView_Previews: PreviewProvider {
    static var previews: some View {
        let testItem = GroceryItem(name: "Apple", barcode: 123456789, category: "Fruits", quantity: 3, location: .fridge, expirationDate: Date())
        return GroceryItemDetailView(item: .constant(testItem))
    }
}

