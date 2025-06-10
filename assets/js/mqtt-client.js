const mqttBroker = "wss://broker.hivemq.com:8884/mqtt";
const topic = "sensor/mq7/CO";

const client = mqtt.connect(mqttBroker);

client.on("connect", () => {
  console.log("Connected to MQTT broker");
  client.subscribe(topic, (err) => {
    if (!err) {
      console.log("Subscribed to topic:", topic);
    } else {
      console.error("Failed to subscribe:", err);
    }
  });
});

client.on("message", (topic, message) => {
  try {
    console.log("Received message:", message.toString());

    const data = JSON.parse(message.toString());
    console.log("Parsed data:", data);

    let coValue;

    if (data.RsRo !== undefined) {
      coValue = data.RsRo.toFixed(2);
    } else if (data.co !== undefined) {
      coValue = data.co;
    } else if (data.value !== undefined) {
      coValue = data.value;
    } else {
      coValue = parseFloat(message.toString());
    }

    console.log("CO Value to display:", coValue);

    // Update CO ppm button
    const coButton = document.getElementById("co-value");
    if (coButton) {
      coButton.textContent = coValue + " ppm";
      console.log("Updated CO button with ID");
    } else {
      console.log("Element with ID 'co-value' not found");

      const coButtons = document.querySelectorAll(".btn.btn-primary.mt-4");
      console.log("Found buttons:", coButtons.length);

      for (let i = 0; i < coButtons.length; i++) {
        if (!coButtons[i].hasAttribute("href")) {
          coButtons[i].textContent = coValue + " ppm";
          console.log("Updated CO button at index", i);
          break;
        }
      }
    }

    // Update CO Status Text
    const coStatus = document.getElementById("co-status");
    if (coStatus) {
      let statusText = "";

      const numericCO = parseFloat(coValue);
      if (numericCO >= 0 && numericCO <= 10) {
        statusText = "Udara Sangat Bersih";
      } else if (numericCO <= 20) {
        statusText = "Aman";
      } else if (numericCO <= 30) {
        statusText = "Waspada";
      } else if (numericCO <= 35) {
        statusText = "Berbahaya";
      } else {
        statusText = "Kritis";
      }

      coStatus.textContent = statusText;
    }

    // Update chart
    const now = new Date().toLocaleTimeString([], {
      hour: "2-digit",
      minute: "2-digit",
    });

    if (typeof coChart !== "undefined") {
      coChart.data.labels.push(now);
      coChart.data.datasets[0].data.push(parseFloat(coValue));

      if (coChart.data.labels.length > 10) {
        coChart.data.labels.shift();
        coChart.data.datasets[0].data.shift();
      }

      coChart.update();
      console.log("Chart updated");
    }
  } catch (error) {
    console.error("Error processing MQTT message:", error);
    console.log("Raw message:", message.toString());
  }
});

client.on("error", (error) => {
  console.error("MQTT connection error:", error);
});

client.on("close", () => {
  console.log("MQTT connection closed");
});

client.on("offline", () => {
  console.log("MQTT client offline");
});
