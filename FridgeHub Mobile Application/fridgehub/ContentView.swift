// ContentView.swift
//           EE 459 Embeded System Laboratory
//                   Spring 2024
//   Created by Team 13: Justin Liu, Richard Chen, Sriya Kuruppath
//                   05/06/2024
//


import SwiftUI
import FirebaseDatabase
import FirebaseDatabaseSwift
struct GroceryItem: Identifiable, Equatable {
    let id = UUID()
    var name: String
    var barcode: Int
    var category: String
    var quantity: Int
    var location: Location
    var expirationDate: Date
}
struct BarcodeLookupView: View {
    @State private var barcode: String = ""
    @State private var lookupResult: GroceryItem?
    @ObservedObject var viewModel: GroceryViewModel // Assuming ViewModel is injected or shared
    
    var body: some View {
        NavigationView {
            VStack {
                TextField("Enter barcode", text: $barcode)
                    .keyboardType(.numberPad)
                    .padding()
        
                Button("Lookup") {
                    performLookup()
                }
                .padding()
                
                if let item = lookupResult {
                    Text("Item: \(item.name)")
                    // Display other item details as needed
                }
            }
            .navigationTitle("Barcode Lookup")
            .padding()
        }
    }
    
    private func performLookup() {
        guard let barcodeInt = Int(barcode) else { return }
        lookupResult = viewModel.itemForBarcode(barcodeInt)
    }
}


enum Location: String {
    case fridge
    case freezer

    init?(rawValue: String) {
        switch rawValue {
        case "fridge":
            self = .fridge
        case "freezer":
            self = .freezer
        default:
            return nil
        }
    }
}


struct ContentView: View {
    @StateObject private var viewModel = GroceryViewModel() // Use StateObject for lifecycle management
    @State private var isAddingNewItem = false
    @State private var isEditing = false
    
    var body: some View {
        TabView {
            NavigationView {
                List {
                    Section(header: Text("Fridge")) {
                        ForEach(viewModel.groceries.filter { $0.location == .fridge && $0.quantity > 0 }) { item in
                            NavigationLink(destination: EditableDetailView(item: Binding(get: { self.viewModel.groceries[self.viewModel.groceries.firstIndex(of: item)!] }, set: { self.viewModel.groceries[self.viewModel.groceries.firstIndex(of: item)!] = $0 }), isEditing: $isEditing, viewModel: viewModel)) {
                                Text(item.name)
                            }
                        }
                        .onDelete(perform: { offsets in
                            let itemNamesToDelete = offsets.map { index in viewModel.groceries.filter { $0.location == .fridge && $0.quantity > 0 }[index].name }
                            viewModel.deleteGroceries(named: itemNamesToDelete)
                        })
                    }
                    Section(header: Text("Freezer")) {
                        ForEach(viewModel.groceries.filter { $0.location == .freezer && $0.quantity > 0 }) { item in
                            NavigationLink(destination: EditableDetailView(item: Binding(get: { self.viewModel.groceries[self.viewModel.groceries.firstIndex(of: item)!] }, set: { self.viewModel.groceries[self.viewModel.groceries.firstIndex(of: item)!] = $0 }), isEditing: $isEditing, viewModel: viewModel)) {
                                Text(item.name)
                            }
                        }
                        .onDelete(perform: { offsets in
                            let itemNamesToDelete = offsets.map { index in viewModel.groceries.filter { $0.location == .freezer && $0.quantity > 0 }[index].name }
                            viewModel.deleteGroceries(named: itemNamesToDelete)
                        })
                    }
                }
                .refreshable {
                    await viewModel.fetchGroceries()
                }
                .navigationTitle("FridgeHub")
                .navigationBarItems(trailing:
                                        Button(action: {
                                            isAddingNewItem = true
                                        }) {
                                            Image(systemName: "plus")
                                        }
                )
            }
            .tabItem {
                Image(systemName: "list.bullet")
                Text("Groceries")
            }
            TemperatureView()
                            .tabItem {
                                Image(systemName: "thermometer")
                                Text("Temperature")
                            }
            
            NotificationsView(viewModel: GroceryViewModel())
                .tabItem {
                    Image(systemName: "bell")
                    Text("Notifications")
                }
            BarcodeLookupView(viewModel: viewModel)
                            .tabItem {
                                Image(systemName: "barcode.viewfinder")
                                Text("Barcode Lookup")
                            }
        }
        .sheet(isPresented: $isAddingNewItem) {
            AddItemView { newItem in
                viewModel.addGroceryItem(newItem)
                isAddingNewItem = false
            }
        }
        
    }
}




class GroceryViewModel: ObservableObject {
    @Published var groceries: [GroceryItem] = []

    private var databaseRef = Database.database().reference().child("groceries")
    // Simulated barcode to item information mapping
    private var barcodeItemMapping: [Int: (name: String, category: String)] = [
        1234567890: (name: "Ice Cream", category: "Frozen Foods"),
        2345678901: (name: "Milk", category: "Dairy"),
        3456789012: (name: "Apples", category: "Fruits"),
        4567890123: (name: "Chicken Breast", category: "Meat"),
        5678901234: (name: "Broccoli", category: "Vegetables"),
        6789012345: (name: "Whole Wheat Bread", category: "Bakery"),
        7890123456: (name: "Orange Juice", category: "Beverages"),
        8901234567: (name: "Eggs", category: "Dairy"),
        9012345678: (name: "Cheddar Cheese", category: "Dairy"),
        1122334455: (name: "Dark Chocolate", category: "Snacks"),
        2233445566: (name: "Greek Yogurt", category: "Dairy"),
        3344556677: (name: "Almonds", category: "Nuts"),
        4455667788: (name: "Salmon Fillets", category: "Seafood"),
        5566778899: (name: "Quinoa", category: "Grains"),
        6677889900: (name: "Spaghetti Pasta", category: "Pasta")
    ]



    private let dateFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd"
        return formatter
    }()

    init() {
        fetchGroceries()
    }

    func fetchGroceries() {
        databaseRef.observe(.value) { snapshot in
            print("Firebase data fetch initiated.")
            
            if snapshot.exists() {
                print("Snapshot exists. Proceeding to parse data.")
                
                var newItems: [GroceryItem] = []
                
                for child in snapshot.children.allObjects as! [DataSnapshot] {
                    let itemName = child.key
                    print("Processing item: \(itemName)")
                    
                    guard let dict = child.value as? [String: Any] else {
                        print("Error: Could not convert data for item '\(itemName)' into a dictionary.")
                        continue
                    }
                    
                    guard let barcode = dict["barcode"] as? Int,
                          let category = dict["category"] as? String,
                          let quantity = dict["quantity"] as? Int,
                          let locationString = dict["location"] as? String,
                          let expirationDateString = dict["expirationDate"] as? String,
                          let location = Location(rawValue: locationString.lowercased()),
                          let expirationDate = self.dateFormatter.date(from: expirationDateString) else {
                        print("Error: Missing or invalid data for item '\(itemName)'.")
                        continue
                    }
                    
                    let groceryItem = GroceryItem(name: itemName, barcode: barcode, category: category, quantity: quantity, location: location, expirationDate: expirationDate)
                    newItems.append(groceryItem)
                    print("Added item: \(itemName)")
                }
                
                DispatchQueue.main.async {
                    self.groceries = newItems
                    print("Successfully updated grocery items.")
                }
            } else {
                print("No data available at the specified Firebase path.")
            }
        }
    }



    func addGroceryItem(_ item: GroceryItem) {
        let itemDict = [
            "barcode": item.barcode,
            "category": item.category,
            "quantity": item.quantity,
            "location": item.location.rawValue,
            "expirationDate": dateFormatter.string(from: item.expirationDate)
        ] as [String : Any]

        // Use the item's name as the key for the item
        databaseRef.child(item.name).setValue(itemDict)
    }
    func deleteGroceries(named names: [String]) {
        for name in names {
            print("Preparing to delete item named: \(name)")  // This will confirm which item is targeted
            deleteGroceryItem(named: name)
        }
    }

    private func deleteGroceryItem(named name: String) {
        let updates = ["quantity": 0]
        databaseRef.child(name).updateChildValues(updates) { error, _ in
            if let error = error {
                print("Error updating item quantity for \(name): \(error.localizedDescription)")
            } else {
                DispatchQueue.main.async {
                    if let index = self.groceries.firstIndex(where: { $0.name == name }) {
                        self.groceries[index].quantity = 0  // This updates the quantity locally
                        self.objectWillChange.send()
                        print("Quantity for \(name) updated to zero.")
                    } else {
                        print("Error: No local item found with the name \(name)")
                    }
                }
            }
        }
    }





    private func updateLocalGroceries(with item: GroceryItem) {
        if let index = groceries.firstIndex(where: { $0.id == item.id }) {
            groceries[index] = item
            objectWillChange.send()
        }
    }



    func itemForBarcode(_ barcode: Int) -> GroceryItem? {
            guard let itemInfo = barcodeItemMapping[barcode] else { return nil }
            
            // Create a GroceryItem with the found info (assuming some default values for missing info)
            return GroceryItem(name: itemInfo.name, barcode: barcode, category: itemInfo.category, quantity: 1, location: .freezer, expirationDate: Date())
        }


}



    private func groceryItem(from snapshot: DataSnapshot) -> GroceryItem? {
        guard
            let value = snapshot.value as? [String: Any],
            let name = value["name"] as? String,
            let barcode = value["barcode"] as? Int,
            let category = value["category"] as? String,
            let quantity = value["quantity"] as? Int,
            let locationString = value["location"] as? String,
            let location = Location(rawValue: locationString),
            let expirationDateString = value["expirationDate"] as? String,
            let expirationDate = dateFormatter.date(from: expirationDateString)
        else {
            return nil
        }

        return GroceryItem(name: name,
                           barcode: barcode,
                           category: category,
                           quantity: quantity,
                           location: location,
                           expirationDate: expirationDate)
    }

    private let dateFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd"
        return formatter
    }()



struct EditableDetailView: View {
    @Binding var item: GroceryItem
    @Binding var isEditing: Bool
    var viewModel: GroceryViewModel
    @Environment(\.presentationMode) var presentationMode
    
    @State private var editedItem: GroceryItem
    
    init(item: Binding<GroceryItem>, isEditing: Binding<Bool>, viewModel: GroceryViewModel) {
        self._item = item
        self._isEditing = isEditing
        self.viewModel = viewModel
        self._editedItem = State(initialValue: item.wrappedValue)
    }
    
    var body: some View {
        VStack {
            Form {
                Section(header: Text("Item Information")) {
                    
                                        TextField("Name", text: $editedItem.name)
                                        TextField("Barcode", value: $editedItem.barcode, formatter: NumberFormatter()).keyboardType(.numberPad)
                                        TextField("Category", text: $editedItem.category)
                                        TextField("Quantity", value: $editedItem.quantity, formatter: NumberFormatter())
                                            .keyboardType(.numberPad)
                                        Picker("Location", selection: $editedItem.location) {
                                            Text("Fridge").tag(Location.fridge)
                                            Text("Freezer").tag(Location.freezer)
                                        }
                                        DatePicker("Expiration Date", selection: $editedItem.expirationDate, displayedComponents: .date)
                                    }
                }
            }
            .navigationTitle("Edit Item")
            
            HStack {
                
                
                Button("Save") {
                    item = editedItem
                    isEditing.toggle()
                    presentationMode.wrappedValue.dismiss()
                }
                .padding()
                
                Button("Cancel") {
                    isEditing.toggle()
                    presentationMode.wrappedValue.dismiss()
                }
                .padding()
            }
            .padding(.bottom)
        }
    }








struct AddItemView: View {
    @Environment(\.presentationMode) var presentationMode
    @State private var name = ""
    @State private var barcode = ""
    @State private var category = ""
    @State private var quantity = ""
    @State private var location = Location.fridge
    @State private var expirationDate = Date()
    
    var addItemAction: ((GroceryItem) -> Void)
    
    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Item Information")) {
                    TextField("Name", text: $name)
                    TextField("Barcode", text: $barcode)
                    TextField("Category", text: $category)
                    TextField("Quantity", text: $quantity)
                        .keyboardType(.numberPad)
                    Picker("Location", selection: $location) {
                        Text("Fridge").tag(Location.fridge)
                        Text("Freezer").tag(Location.freezer)
                    }
                    DatePicker("Expiration Date", selection: $expirationDate, displayedComponents: .date)
                }
                
                Section {
                    HStack {
                        Spacer()
                        Button("Cancel") {
                            presentationMode.wrappedValue.dismiss()
                        }
                        Spacer()
                        Button("Add Item") {
                            addItem()
                        }
                        Spacer()
                    }
                }
            }
            .navigationTitle("Add Item")
        }
    }
    
    private func addItem() {
        guard let quantity = Int(quantity) else { return }
        guard let barcode = Int(barcode) else { return }
        
        let newItem = GroceryItem(name: name, barcode: barcode, category: category, quantity: quantity, location: location, expirationDate: expirationDate)
        addItemAction(newItem)
        presentationMode.wrappedValue.dismiss() // Dismiss the sheet after adding the item
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
