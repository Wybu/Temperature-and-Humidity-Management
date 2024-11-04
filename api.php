<?php
// Database configuration
$servername = "localhost";
$username = "root";
$password = "";
$dbname = "Iot_data";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Get the request method
$method = $_SERVER['REQUEST_METHOD'];

// If the request method is POST, handle data insertion
if ($method == 'POST') {
    // Get the JSON data from the request body
    $data = json_decode(file_get_contents("php://input"), true);

    // Check if the data is valid
    if (isset($data['temperature'], $data['humidity'], $data['soil_moisture'], $data['light_level'])) {
        $temperature = $data['temperature'];
        $humidity = $data['humidity'];
        $soil_moisture = $data['soil_moisture'];
        $light_level = $data['light_level'];

        // Prepare and bind the SQL statement
        $stmt = $conn->prepare("INSERT INTO data (temperature, humidity, soil_moisture, light_level) VALUES (?, ?, ?, ?)");
        $stmt->bind_param("ddii", $temperature, $humidity, $soil_moisture, $light_level);

        // Execute the statement
        if ($stmt->execute()) {
            echo json_encode(["status" => "success", "message" => "Data inserted successfully"]);
        } else {
            echo json_encode(["status" => "error", "message" => "Failed to insert data"]);
        }

        $stmt->close();
    } else {
        echo json_encode(["status" => "error", "message" => "Invalid data"]);
    }
} 
// If the request method is GET, retrieve all data ordered by timestamp
elseif ($method == 'GET') {
    // Select all data ordered by timestamp in descending order (newest first)
    $sql = "SELECT * FROM data ORDER BY timestamp DESC";
    $result = $conn->query($sql);

    if ($result->num_rows > 0) {
        $allData = [];

        while ($row = $result->fetch_assoc()) {
            $allData[] = $row;
        }

        echo json_encode(["status" => "success", "data" => $allData]);
    } else {
        echo json_encode(["status" => "error", "message" => "No data found"]);
    }
} else {
    echo json_encode(["status" => "error", "message" => "Unsupported request method"]);
}

// Close the connection
$conn->close();
?>