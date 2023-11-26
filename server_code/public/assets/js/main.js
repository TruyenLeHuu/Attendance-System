$.when(
  $.getScript('./configClient/config.js',function(){
      Socket_hostIP = hostIP;
      Socket_port = port;
  })).done(function(){
  var temperatureData = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,] ; 
  var humidityData = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,]; 
  var smokeData = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,]; 
  const temperatureCanvas = document.getElementById('temperatureChart');
  const humidityCanvas = document.getElementById('humidityChart');
  const smokeCanvas = document.getElementById('smokeChart');

  const tempValue = document.getElementById('currentTemperature');
  const humValue = document.getElementById('currentHumidity');
  const smokeValue = document.getElementById('currentSmoke');

  const statusButton = document.getElementById('status-btn');

  const ctx1 = temperatureCanvas.getContext('2d');
  const ctx2 = humidityCanvas.getContext('2d');
  const ctx3 = smokeCanvas.getContext('2d');

  var num_danger = 0;
  var num_warning = 0;

  const connectionObject = {
    withCredentials: true,
  };
  const socket = io("", connectionObject);
  // socket = io.connect('http://' + Socket_hostIP + ':' + Socket_port, { transports : ['websocket'] });
  socket.emit('Get-data');
  socket.on('Environment-update', (data)=>{
    obj_data = JSON.parse(data);
    socket.emit('Get-data');
    if (obj_data.status == 1){
      num_warning = (num_warning == 3) ? 3 : (num_warning +1);
      num_danger = (num_danger == 0) ? 0 : (num_danger - 1);
    }
    else if (obj_data.status == 2) 
      num_danger = (num_danger == 3) ? 3 : (num_danger +1);
    else {
      num_danger = (num_danger == 0) ? 0 : (num_danger - 1);
      num_warning = (num_warning == 0) ? 0 : (num_warning - 1);
    }
    if (num_danger > 0) {
      $("#status-btn").removeClass("btn-primary").removeClass("btn-warning").addClass("btn-danger");
      statusButton.textContent = "Fire!!!";
      socket.emit("Signal-fire")
    } else if (num_warning > 0){
      $("#status-btn").removeClass("btn-primary").removeClass("btn-danger").addClass("btn-warning");
      statusButton.textContent = "Warning";
      socket.emit("Signal-normal")
    } else {
      $("#status-btn").removeClass("btn-warning").removeClass("btn-danger").addClass("btn-primary");
      statusButton.textContent = "Normal";
      socket.emit("Signal-normal")
    }
    console.log(num_warning + " " + num_danger)
    })
  socket.on('Send-data', (data)=>{
    
    // console.info(data)
    tempValue.textContent = data[0].temp.toFixed(2) + "°C";
    humValue.textContent = data[0].hum.toFixed(2) + "%";
    smokeValue.textContent = data[0].smoke.toFixed(2) + "mA";
    data.forEach((env, index) => {
      temperatureData[index] = env.temp;
      humidityData[index] = env.hum;
      smokeData[index] = env.smoke;
    });
    temperatureChart.data.datasets[0].data = temperatureData;
    temperatureChart.update();

    humidityChart.data.datasets[0].data = humidityData;
    humidityChart.update();

    smokeChart.data.datasets[0].data = smokeData;
    smokeChart.update();
  })
  statusButton.addEventListener('click', 
  ()=>{
        socket.emit('Buzzer');
    })
  const temperatureChart = new Chart(ctx1, {
    type: 'line',
    data: {
      labels: Array.from({ length: temperatureData.length }, (_, i) => i + 1),
      datasets: [{
        label: 'Temperature',
        data: temperatureData,
        fill: false,
        borderColor: '#ff6384',
        borderWidth: 2
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: false
        }
      },
      plugins: {
        legend: {
          display: false
        },
        title: {
          display: true,
          text: 'Temprature (°C)'
        }
      }
      
    }
  });

  const humidityChart = new Chart(ctx2, {
    type: 'line',
    data: {
      labels: Array.from({ length: humidityData.length }, (_, i) => i + 1),
      datasets: [{
        label: 'Humidity',
        data: humidityData,
        fill: false,
        borderColor: '#36a2eb',
        borderWidth: 2
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: false
        }
      },
      plugins: {
        legend: {
          display: false
        },
        title: {
          display: true,
          text: 'Humidity (%)'
        }
      }
    }
  });

  const smokeChart = new Chart(ctx3, {
    type: 'line',
    data: {
      labels: Array.from({ length: smokeData.length }, (_, i) => i + 1),
      datasets: [{
        label: 'Smoke',
        data: smokeData,
        fill: false,
        borderColor: '#36a2eb',
        borderWidth: 2
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: false
        }
      },
      plugins: {
        legend: {
          display: false
        },
        title: {
          display: true,
          text: 'Smoke (mA)'
        }
      }
    }
  });
})
