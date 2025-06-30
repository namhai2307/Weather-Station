let temperature = document.getElementById('temperature');
let humidity = document.getElementById('humidity');
let pressure = document.getElementById('pressure');
let wind_speed = document.getElementById('wind_speed');
let light_intensity = document.getElementById('light_intensity');
let uv_index = document.getElementById('uv_index');
let rain_volume = document.getElementById('rain');

let url = 'https://weather.visualcrossing.com/VisualCrossingWebServices/rest/services/retrievebulkdataset?&key=2LL9YL3YZTTEBB7RBHTWPFM2U&taskId=6f5592ae5db668da13b68a80928d4984&zip=false';
let esp_url = 'http://192.168.1.167/sensor';

const ctx_humid = document.getElementById('humidChart').getContext('2d');
let humidData = [];
let humidLabels = [];

let humidChart = new Chart(ctx_humid, {
    type: 'line',
    data: {
        labels: humidLabels,
        datasets: [{
            label: 'Humidity last hour',
            data: humidData,
            borderColor: 'blue',
            borderWidth: 2,
            fill: false,
        }]
    },
    options: {
        responsive: true,
        scales: {
            x: { 
                title: { display: true, text: 'Time' },
                ticks: { autoSkip: true, maxTicksLimit: 6 }
            },
            y: {
                title: { display: true, text: 'Humidity (%)' },
                beginAtZero: false
            }
        }
    }
});


const Fetch_weather = () => {
    fetch(esp_url)
    .then(responsive => responsive.json())
    .then(data => {
        //console.log(data);
        // Update elements
        temperature.innerText = data.temperature;
        wind_speed.innerText = data.wind_speed;
        humidity.innerText = data.humidity;
        uv_index.innerText = data.uv_index;
        light_intensity.innerText = data.lux;
        pressure.innerText = data.air_pressure;
        rain_volume.innerText = data.rain_volume;

        // Update chart data
        if (humidData.length >= 6) {
            humidData.shift(); // Remove oldest entry
            humidLabels.shift();
        }
        humidData.push(data.humidity);
        humidLabels.push(currentTime);
        humidChart.update();
        return data;
    })
}

function UpdateData() {

}

//Fetch new data every 10 seconds
setInterval(Fetch_weather, 10000);