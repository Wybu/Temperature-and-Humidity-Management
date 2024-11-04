const clientId = "web_" + Math.random().toString(16).substr(2, 8);
const client = mqtt.connect(
  "wss://e5abb49a32fb400ca6e7c5dda3604fd4.s1.eu.hivemq.cloud:8884/mqtt",
  {
    clientId: clientId,
    username: "hung",
    password: "hung",
    clean: true,
    rejectUnauthorized: false,
    keepalive: 60,
  }
);
client.on("connect", () => {
  console.log("Đã kết nối với MQTT broker");
  // Cập nhật thông báo kết nối
  const connectionStatus = document.getElementById("connectionStatus");
  connectionStatus.innerText = "Đã kết nối thành công!";
  connectionStatus.style.color = "green"; // Chữ xanh

  client.subscribe("iot/sensors", (err) => {
    if (err) {
      console.error("Không thể đăng ký topic:", err);
    } else {
      console.log("Đã đăng ký topic: iot/sensors");
    }
  });
});

client.on("error", (err) => {
  console.log("Lỗi kết nối:", err);
  // Cập nhật thông báo kết nối thất bại
  const connectionStatus = document.getElementById("connectionStatus");
  connectionStatus.innerText = "Không kết nối được!";
  connectionStatus.style.color = "red"; // Chữ đỏ
});

let data = {};
let controlRelay = {};
let latestData = {}; // To store the latest sensor data
client.on("message", (topic, message) => {
  data = JSON.parse(message.toString());
  latestData = JSON.parse(message.toString());
  document.getElementById("temperature").innerText = data.temperature;
  document.getElementById("humidity").innerText = data.humidity;
  document.getElementById("soil_moisture").innerText = data.soil_moisture;
  document.getElementById("light_level").innerText = data.light_level;
  const autoSwitch = document.getElementById("autoSwitch");
  if (autoSwitch.checked) {
    document.getElementById("pumpSwitch").checked = data.pump_state;
    document.getElementById("fanSwitch").checked = data.fan_state;
    document.getElementById("ledSwitch").checked = data.led_state;
    controlRelay = {
      fan: data.fan_state,
      pump: data.pump_state,
      led: data.led_state,
    };
  }
  document.getElementById("displayTemperature").innerText =
    data.threshold_temperature;
  document.getElementById("displayHumidity").innerText =
    data.threshold_humidity;
  document.getElementById("displaySoilMoisture").innerText =
    data.threshold_soilMoisture;
  document.getElementById("displayLightLevel").innerText =
    data.threshold_lightLevel;
});

const intervalId = setInterval(() => {
  if (latestData && Object.keys(latestData).length > 0) {
    fetch("http://localhost:8000/api.php", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        temperature: latestData.temperature,
        humidity: latestData.humidity,
        soil_moisture: latestData.soil_moisture,
        light_level: latestData.light_level,
      }),
    })
      .then((response) => {
        if (!response.ok) {
          throw new Error("Network response was not ok");
        }
        return response.json(); // Nếu server trả về JSON
      })
      .then((data) => {
        console.log("Data sent to server:", data);
      })
      .catch((error) => {
        console.error("Error sending data:", error);
        // Có thể thêm logic thử lại ở đây nếu cần
      });
  }
}, 10000); // 10 seconds interval

// Hàm để lấy dữ liệu từ server và hiển thị lên bảng
function fetchData() {
  fetch("http://localhost:8000/api.php") // Đổi 'path/to/your_php_file.php' thành đường dẫn đến file PHP của bạn
    .then((response) => response.json())
    .then((data) => {
      // Xác định vị trí chèn dữ liệu vào bảng
      const tableBody = document.getElementById("dataTableBody");
      tableBody.innerHTML = ""; // Xóa dữ liệu cũ nếu có

      if (data.status === "success") {
        data.data.forEach((row) => {
          // Tạo hàng cho mỗi bản ghi
          const tr = document.createElement("tr");

          // Chèn dữ liệu vào các ô trong bảng
          tr.innerHTML = `
                            <td>${new Date(row.timestamp).toLocaleString()}</td>
                            <td>${row.temperature}</td>
                            <td>${row.humidity}</td>
                            <td>${row.soil_moisture}</td>
                            <td>${row.light_level}</td>
                        `;
          tableBody.appendChild(tr); // Thêm hàng vào bảng
        });
      } else {
        tableBody.innerHTML = "<tr><td colspan='5'>Không có dữ liệu</td></tr>";
      }
    })
    .catch((error) => console.error("Error fetching data:", error));
}

// Gọi hàm fetchData khi tải trang hoặc khi cần thiết
document.addEventListener("DOMContentLoaded", fetchData);

// (Tuỳ chọn) Đặt hàm fetchData để tự động cập nhật mỗi X giây, ví dụ: 5 giây
setInterval(fetchData, 5000);

document.addEventListener("DOMContentLoaded", function () {
  const autoSwitch = document.getElementById("autoSwitch");
  const fanSwitch = document.getElementById("fanSwitch");
  const pumpSwitch = document.getElementById("pumpSwitch");
  const ledSwitch = document.getElementById("ledSwitch");

  autoSwitch.addEventListener("change", function () {
    if (autoSwitch.checked) {
      client.publish("iot/mode", JSON.stringify({ auto: true }), {
        qos: 0,
        retain: false,
      });
      fanSwitch.disabled = true;
      pumpSwitch.disabled = true;
      ledSwitch.disabled = true;
      document.getElementById("pumpSwitch").checked = data.pump_state;
      document.getElementById("fanSwitch").checked = data.fan_state;
      document.getElementById("ledSwitch").checked = data.led_state;
    } else {
      client.publish("iot/mode", JSON.stringify({ auto: false }), {
        qos: 0,
        retain: false,
      });
      controlRelay = {
        fan: data.fan_state,
        pump: data.pump_state,
        led: data.led_state,
      };
      console.log(controlRelay);
      fanSwitch.disabled = false;
      pumpSwitch.disabled = false;
      ledSwitch.disabled = false;
    }
  });
});

function handleToggleChange(checkbox) {
  const switchId = checkbox.getAttribute("data-id");
  const isChecked = checkbox.checked;
  console.log(switchId);
  if (switchId != "auto") {
    if (switchId === "fan") {
      controlRelay.fan = isChecked;
    }
    if (switchId === "pump") {
      controlRelay.pump = isChecked;
    }
    if (switchId === "led") {
      controlRelay.led = isChecked;
    }
    console.log(controlRelay);
    client.publish("iot/control", JSON.stringify(controlRelay), {
      qos: 0,
      retain: false,
    });
  }
}

document
  .getElementById("sensorForm")
  .addEventListener("submit", function (event) {
    event.preventDefault(); // Ngăn chặn hành động gửi form

    // Lấy giá trị từ form
    const temperature = document.getElementById("threshold_temperature").value;
    const humidity = document.getElementById("threshold_humidity").value;
    const soilMoisture = document.getElementById(
      "threshold_soilMoisture"
    ).value;
    const lightLevel = document.getElementById("threshold_lightLevel").value;

    // Cập nhật thông tin hiển thị
    const data = {
      threshold_temperature: temperature,
      threshold_humidity: humidity,
      threshold_soilMoisture: soilMoisture,
      threshold_lightLevel: lightLevel,
    };
    console.log(data);
    client.publish("iot/thresholds", JSON.stringify(data), {
      qos: 0,
      retain: false,
    });
    // Reset form sau khi cài đặt
    document.getElementById("sensorForm").reset();
  });
