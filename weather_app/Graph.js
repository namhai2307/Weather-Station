import { Fetch_weather } from './App.js';
import Chart from 'chart.js/auto';

// Function to create a chart instance
const createChart = (ctx, label, borderColor) => {
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: label,
                data: [],
                borderColor: borderColor,
                borderWidth: 2,
                fill: false,
                tension: 0.1,
            }]
        },
        options: {
            responsive: true,
            scales: {
                x: {
                    title: {
                        display: true,
                        text: 'Timestamp',
                    },
                    type: 'time',
                    time: {
                        unit: 'second',
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: label,
                    },
                    beginAtZero: true
                }
            }
        }
    });
};

// Initialize charts for each weather parameter
const charts = {
    temperature: createChart(document.getElementById('tempChart').getContext('2d'), 'Temperature (°C)', 'rgb(255, 0, 0)'),
    wind_speed: createChart(document.getElementById('windChart').getContext('2d'), 'Wind Speed (m/s)', 'rgb(0, 0, 255)'),
    humidity: createChart(document.getElementById('humidChart').getContext('2d'), 'Humidity (%)', 'rgb(0, 255, 0)'),
    uv_index: createChart(document.getElementById('UVChart').getContext('2d'), 'UV Index', 'rgb(255, 165, 0)'),
    light_intensity: createChart(document.getElementById('luxChart').getContext('2d'), 'Light Intensity (lux)', 'rgb(255, 255, 0)'),
    pressure: createChart(document.getElementById('pressChart').getContext('2d'), 'Pressure (Pa)', 'rgb(128, 0, 128)')
};

// Function to update all charts with new data
const updateCharts = async () => {
    const now = new Date().toLocaleTimeString(); // Current time
    const data = await Fetch_weather(); // Fetch new data

    Object.keys(charts).forEach(param => {
        if (data[param] !== undefined) {
            const value = parseFloat(data[param]);
            charts[param].data.labels.push(now);
            charts[param].data.datasets[0].data.push(value);

            // Limit the number of data points displayed
            if (charts[param].data.labels.length > 20) {
                charts[param].data.labels.shift();
                charts[param].data.datasets[0].data.shift();
            }

            charts[param].update(); // Update the chart
        }
    });
};

// Update charts every 10 seconds
setInterval(updateCharts, 10000);
