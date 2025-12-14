// HTML page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <title>Hệ Thống Giám Sát & Cảnh Báo Cháy</title>
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }

        .header {
            text-align: center;
            margin-bottom: 30px;
            animation: fadeInDown 0.8s ease;
        }

        .header h1 {
            color: white;
            font-size: 2.5rem;
            font-weight: 700;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.2);
            margin-bottom: 10px;
        }

        .header p {
            color: rgba(255, 255, 255, 0.9);
            font-size: 1.1rem;
        }

        .dashboard {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 20px;
            max-width: 1200px;
            width: 100%;
            margin-bottom: 20px;
        }

        .card {
            background: white;
            border-radius: 20px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
            animation: fadeInUp 0.8s ease;
            position: relative;
            overflow: hidden;
        }

        .card::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 4px;
            background: linear-gradient(90deg, var(--card-color), var(--card-color-light));
        }

        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 15px 40px rgba(0, 0, 0, 0.25);
        }

        .card-header {
            display: flex;
            align-items: center;
            margin-bottom: 15px;
        }

        .card-icon {
            width: 50px;
            height: 50px;
            border-radius: 12px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.8rem;
            margin-right: 15px;
            background: var(--card-color);
            color: white;
        }

        .card-title {
            font-size: 1.1rem;
            color: #555;
            font-weight: 600;
        }

        .card-value {
            font-size: 2.5rem;
            font-weight: 700;
            color: #222;
            margin: 10px 0;
        }

        .card-unit {
            font-size: 1rem;
            color: #888;
            font-weight: 500;
        }

        .card.temperature {
            --card-color: #ff6b6b;
            --card-color-light: #ff8787;
        }

        .card.humidity {
            --card-color: #4ecdc4;
            --card-color-light: #6ee2d9;
        }

        .card.gas {
            --card-color: #95a5a6;
            --card-color-light: #b0bec5;
        }

        .card.chart {
            --card-color: #f39c12;
            --card-color-light: #f1c40f;
            grid-column: span 1;
        }

        .card.door {
            --card-color: #3498db;
            --card-color-light: #5dade2;
            text-align: center;
        }

        .door-btn {
            width: 100%;
            padding: 18px;
            border-radius: 15px;
            border: none;
            font-size: 1.3rem;
            font-weight: 700;
            cursor: pointer;
            transition: all 0.3s ease;
            color: white;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 12px;
        }

        .door-btn.closed {
            background: linear-gradient(135deg, #7f8c8d, #636e72);
        }

        .door-btn.open {
            background: linear-gradient(135deg, #2ecc71, #27ae60);
        }

        .door-btn:hover {
            transform: scale(1.03);
        }

        .card.flame {
            --card-color: #e74c3c;
            --card-color-light: #ff6b6b;
        }

        .flame-status {
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
            border-radius: 15px;
            margin-top: 15px;
            transition: all 0.3s ease;
        }

        .flame-status.normal {
            background: linear-gradient(135deg, #2ecc71, #27ae60);
            color: white;
        }

        .flame-status.warning {
            background: linear-gradient(135deg, #e74c3c, #c0392b);
            color: white;
            animation: pulse 1.5s infinite;
        }

        .status-text {
            font-size: 1.5rem;
            font-weight: 700;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .status-icon {
            font-size: 2rem;
        }

        .alert-section {
            background: white;
            border-radius: 20px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            max-width: 1200px;
            width: 100%;
            text-align: center;
            animation: fadeInUp 1s ease;
        }

        .alert-status {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 15px;
            padding: 18px 50px;
            border-radius: 50px;
            font-size: 1.3rem;
            font-weight: 700;
            transition: all 0.3s ease;
            box-shadow: 0 8px 20px rgba(0, 0, 0, 0.15);
        }

        .alert-status.safe {
            background: linear-gradient(135deg, #2ecc71, #27ae60);
            color: white;
        }

        .alert-status.danger {
            background: linear-gradient(135deg, #e74c3c, #c0392b);
            color: white;
            animation: pulse 1.5s infinite, shake 0.5s infinite;
        }

        .alert-icon {
            font-size: 2rem;
        }

        .alert-text {
            letter-spacing: 1px;
        }

        #sensorChart {
            max-height: 200px;
        }

        @keyframes fadeInDown {
            from {
                opacity: 0;
                transform: translateY(-30px);
            }

            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        @keyframes fadeInUp {
            from {
                opacity: 0;
                transform: translateY(30px);
            }

            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        @keyframes pulse {

            0%,
            100% {
                transform: scale(1);
            }

            50% {
                transform: scale(1.03);
            }
        }

        @keyframes shake {

            0%,
            100% {
                transform: translateX(0);
            }

            25% {
                transform: translateX(-3px);
            }

            75% {
                transform: translateX(3px);
            }
        }

        @media (max-width: 992px) {
            .dashboard {
                grid-template-columns: repeat(2, 1fr);
            }

            .card.flame {
                grid-column: span 2;
            }

            .card.chart {
                grid-column: span 2;
            }
        }

        @media (max-width: 768px) {
            .header h1 {
                font-size: 2rem;
            }

            .dashboard {
                grid-template-columns: 1fr;
            }

            .card.flame {
                grid-column: span 1;
            }

            .card.chart {
                grid-column: span 1;
            }

            .card-value {
                font-size: 2rem;
            }

            .status-text {
                font-size: 1.2rem;
            }

            .alert-status {
                font-size: 1.1rem;
                padding: 15px 30px;
            }
        }
    </style>
</head>

<body>
    <div class="header">
        <h1><i class="fas fa-fire-extinguisher"></i> Hệ Thống Giám Sát Cháy & Rò Rỉ Khí Gas</h1>
        <p>Theo dõi nhiệt độ, độ ẩm, khí gas và phát hiện lửa</p>
    </div>

    <div class="dashboard">
        <div class="card temperature">
            <div class="card-header">
                <div class="card-icon">
                    <i class="fas fa-thermometer-half"></i>
                </div>
                <div class="card-title">Nhiệt độ</div>
            </div>
            <div class="card-value" id="temperature">--</div>
            <div class="card-unit">°C</div>
        </div>

        <div class="card humidity">
            <div class="card-header">
                <div class="card-icon">
                    <i class="fas fa-tint"></i>
                </div>
                <div class="card-title">Độ ẩm</div>
            </div>
            <div class="card-value" id="humidity">--</div>
            <div class="card-unit">%</div>
        </div>

        <div class="card gas">
            <div class="card-header">
                <div class="card-icon">
                    <i class="fas fa-cloud"></i>
                </div>
                <div class="card-title">Khí Gas</div>
            </div>
            <div class="card-value" id="gas">--</div>
            <div class="card-unit">Mức khói (ADC)</div>
        </div>

        <div class="card chart">
            <div class="card-header">
                <div class="card-icon">
                    <i class="fas fa-chart-line"></i>
                </div>
                <div class="card-title">Biểu đồ theo dõi</div>
            </div>
            <canvas id="sensorChart"></canvas>
        </div>

        <div class="card door">
            <div class="card-header">
                <div class="card-icon">
                    <i class="fas fa-door-open"></i>
                </div>
                <div class="card-title">Cửa thoát hiểm</div>
            </div>

            <button class="door-btn closed" id="doorBtn" onclick="toggleDoor()">
                <i class="fas fa-lock"></i>
                ĐANG ĐÓNG
            </button>
        </div>

        <div class="card flame">
            <div class="card-header">
                <div class="card-icon">
                    <i class="fas fa-fire"></i>
                </div>
                <div class="card-title">Cảnh báo lửa</div>
            </div>
            <div class="flame-status normal" id="flameStatus">
                <div class="status-text">
                    <span id="flameText">An toàn</span>
                </div>
            </div>
        </div>
    </div>

    <div class="alert-section">
        <div class="alert-status safe" id="alertStatus">
            <i class="fas fa-shield-alt alert-icon"></i>
            <span class="alert-text">HỆ THỐNG AN TOÀN</span>
        </div>
    </div>

    <script>
        // Khởi tạo biểu đồ
        const ctx = document.getElementById('sensorChart').getContext('2d');
        const maxDataPoints = 20; // Hiển thị 20 điểm dữ liệu (40 giây)

        const chartData = {
            labels: [],
            datasets: [
                {
                    label: 'Nhiệt độ (°C)',
                    data: [],
                    borderColor: '#ff6b6b',
                    backgroundColor: 'rgba(255, 107, 107, 0.1)',
                    tension: 0.4,
                    borderWidth: 2,
                    pointRadius: 3,
                    pointHoverRadius: 5
                },
                {
                    label: 'Độ ẩm (%)',
                    data: [],
                    borderColor: '#4ecdc4',
                    backgroundColor: 'rgba(78, 205, 196, 0.1)',
                    tension: 0.4,
                    borderWidth: 2,
                    pointRadius: 3,
                    pointHoverRadius: 5
                },
                    
            ]
        };

        const chart = new Chart(ctx, {
            type: 'line',
            data: chartData,
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    legend: {
                        display: true,
                        position: 'top',
                        labels: {
                            font: {
                                size: 11,
                                weight: 'bold'
                            },
                            padding: 10,
                            usePointStyle: true
                        }
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        max: 100,
                        grid: {
                            color: 'rgba(0, 0, 0, 0.05)'
                        },
                        ticks: {
                            font: {
                                size: 10
                            }
                        }
                    },
                    x: {
                        grid: {
                            display: false
                        },
                        ticks: {
                            font: {
                                size: 9
                            },
                            maxRotation: 0
                        }
                    }
                },
                interaction: {
                    intersect: false,
                    mode: 'index'
                }
            }
        });

        function updateChart(temperature, humidity) {
            const now = new Date();
            const timeLabel = now.getHours().toString().padStart(2, '0') + ':' + 
                            now.getMinutes().toString().padStart(2, '0') + ':' + 
                            now.getSeconds().toString().padStart(2, '0');

            chartData.labels.push(timeLabel);
            chartData.datasets[0].data.push(temperature);
            chartData.datasets[1].data.push(humidity);

            // Giới hạn số điểm dữ liệu
            if (chartData.labels.length > maxDataPoints) {
                chartData.labels.shift();
                chartData.datasets[0].data.shift();
                chartData.datasets[1].data.shift();
            }

            chart.update('none'); // Cập nhật không có animation để mượt hơn
        }

        function updateSensorData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temperature;
                    document.getElementById('humidity').textContent = data.humidity;
                    document.getElementById('gas').textContent = data.gas;

                    // Cập nhật biểu đồ
                    updateChart(parseFloat(data.temperature), parseFloat(data.humidity));

                    const flameStatus = document.getElementById('flameStatus');
                    const flameText = document.getElementById('flameText');
                    const alertStatus = document.getElementById('alertStatus');

                    if (data.fireAlert) {
                        flameStatus.className = 'flame-status warning';
                        flameText.innerHTML = '<i class="fas fa-exclamation-triangle status-icon"></i>CẢNH BÁO';
                        alertStatus.className = 'alert-status danger';
                        alertStatus.innerHTML = '<i class="fas fa-exclamation-triangle alert-icon"></i><span class="alert-text">CẢNH BÁO KHẨN CẤP</span>';
                    } else {
                        flameStatus.className = 'flame-status normal';
                        flameText.innerHTML = '<i class="fas fa-check-circle status-icon"></i>An toàn';
                        alertStatus.className = 'alert-status safe';
                        alertStatus.innerHTML = '<i class="fas fa-shield-alt alert-icon"></i><span class="alert-text">HỆ THỐNG AN TOÀN</span>';
                    }

                    // Cập nhật trạng thái cửa từ ESP32
                    const doorBtn = document.getElementById('doorBtn');
                    if (data.doorOpen) {
                        doorBtn.className = 'door-btn open';
                        doorBtn.innerHTML = '<i class="fas fa-door-open"></i> ĐANG MỞ';
                    } else {
                        doorBtn.className = 'door-btn closed';
                        doorBtn.innerHTML = '<i class="fas fa-lock"></i> ĐANG ĐÓNG';
                    }
                })
                .catch(error => console.error('Error:', error));
        }

        setInterval(updateSensorData, 2000);
        updateSensorData();

        function toggleDoor() {
            const btn = document.getElementById('doorBtn');

            if (btn.classList.contains('closed')) {
                // Cập nhật UI ngay lập tức
                btn.className = 'door-btn open';
                btn.innerHTML = '<i class="fas fa-door-open"></i> ĐANG MỞ';
                fetch('/door?state=open');
            } else {
                // Cập nhật UI ngay lập tức
                btn.className = 'door-btn closed';
                btn.innerHTML = '<i class="fas fa-lock"></i> ĐANG ĐÓNG';
                fetch('/door?state=close');
            }
        }
    </script>
</body>
</html>
)rawliteral";