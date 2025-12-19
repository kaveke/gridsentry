/**
 * Add globals here
 */
let wifiConnectStatusInterval = null;
let networkIntervalId = null;
let statusMessageTimeout = null;

/**
 * Initialize functions here.
 */
$(document).ready(function () {
    console.log("jQuery has been loaded successfully");
    
    // Initialize event listeners
    $("#file_select").on("click", function() {
        $("#selected_file").click();
    });
    
    $("#selected_file").on("change", getFileInfo);
    $("#upload_firmware").on("click", updateFirmware);
    $("#connect_Wifi").on("click", checkCredentials);
    $("#show_password").on("change", showPassword);
    $("#disconnect_wifi").on("click", disconnectWifi);
    
    // Initialize the ESP32 dashboard
    initializeESP32();
});

// Custom event for connection status changes
window.addEventListener('esp32-connection-change', function(event) {
    updateConnectionUI(event.detail?.connected);
});

/**
 * Initialize the ESP32 dashboard
 */
function initializeESP32() {
    console.log("Initializing ESP32 dashboard...");
    
    // Get initial data
    getUpdateStatus();
    getConnectInfo();
    
    console.log("ESP32 dashboard initialized");
}

/**
 * Updates the UI based on connection status
 */
function updateConnectionUI(isConnected) {
    console.log("Updating connection UI, connected:", isConnected);

    const connectionIndicator = $('#connection-indicator .connection-status');
    const wifiConnectCard = $('#WiFiConnect');
    const connectionInfoDisplay = $('#connection-info-display');
    const notConnectedMessage = $('#not-connected-message');

    if (isConnected) {
        connectionIndicator.removeClass('offline').addClass('online');
        connectionIndicator.html('<i class="fas fa-circle-check mr-2 text-secondary"></i><span>Connected</span>');
        wifiConnectCard.hide();
        connectionInfoDisplay.removeClass('hidden').addClass('block');
        notConnectedMessage.addClass('hidden');

        $('#node-device').addClass('node-glow');
        $('#node-wifi').addClass('node-glow pulse');
        $('#node-internet').addClass('node-glow');
        $('#line-wifi').addClass('bg-secondary line-glow');
        $('#line-internet').addClass('bg-secondary line-glow');

        // Start SNTP time updates after connection
        startLocalTimeInterval();

    } else {
        connectionIndicator.removeClass('online').addClass('offline');
        connectionIndicator.html('<i class="fas fa-circle-exclamation mr-2 text-destructive"></i><span>Not Connected</span>');
        wifiConnectCard.show();
        connectionInfoDisplay.addClass('hidden').removeClass('block');
        notConnectedMessage.removeClass('hidden');

        $('#node-device').removeClass('node-glow');
        $('#node-wifi').removeClass('node-glow pulse');
        $('#node-internet').removeClass('node-glow');
        $('#line-wifi').removeClass('bg-secondary line-glow');
        $('#line-internet').removeClass('bg-secondary line-glow');

        // Stop SNTP updates if disconnected
        if (networkIntervalId) {
            clearInterval(networkIntervalId);
            networkIntervalId = null;
        }
    }
}

/**
 * Shows a status message with the given text and color.
 */
function showStatusMessage(message, color = 'black', duration = 3000) {
    // Clear existing timeout if one exists
    if (statusMessageTimeout) {
        clearTimeout(statusMessageTimeout);
    }
    
    const statusElement = document.getElementById('status_message');
    
    // Set message and styles
    statusElement.textContent = message;
    statusElement.style.color = color;
    statusElement.classList.remove('translate-y-full');
    statusElement.classList.add('translate-y-0');
    
    // Set timeout to hide the message
    statusMessageTimeout = setTimeout(function() {
        statusElement.classList.remove('translate-y-0');
        statusElement.classList.add('translate-y-full');
    }, duration);
}

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() {
    const input = document.getElementById('selected_file');
    const fileInfoDiv = document.getElementById('file_info');
    const uploadButton = document.getElementById('upload_firmware');
    
    if (input.files.length === 0) {
        fileInfoDiv.textContent = '';
        uploadButton.disabled = true;
        return;
    }
    
    const file = input.files[0];
    const fileName = file.name;
    let fileSize = file.size;
    
    // Convert file size to readable format
    let fileSizeStr = fileSize + ' bytes';
    if (fileSize > 1024) {
        fileSizeStr = (fileSize / 1024).toFixed(2) + ' KB';
    }
    if (fileSize > 1024 * 1024) {
        fileSizeStr = (fileSize / (1024 * 1024)).toFixed(2) + ' MB';
    }
    
    fileInfoDiv.textContent = fileName + ' (' + fileSizeStr + ')';
    uploadButton.disabled = false;
}

/**
 * Handles the firmware update.
 */
function updateFirmware() {
    const fileInput = document.getElementById('selected_file');
    if (!fileInput.files.length) {
        showStatusMessage('No file selected!', 'red');
        return;
    }
    
    const file = fileInput.files[0];
    const xhr = new XMLHttpRequest();
    const progressBar = document.getElementById('prg');
    const uploadButton = document.getElementById('upload_firmware');
    const otaStatusDiv = document.getElementById('ota_update_status');
    
    // Prepare form data
    const formData = new FormData();
    formData.append('firmware', file, file.name);
    
    // Configure request
    xhr.open('POST', '/OTAupdate');
    xhr.upload.addEventListener('progress', updateProgress);
    
    // Disable the button during upload
    uploadButton.disabled = true;
    
    // Handle response
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                showStatusMessage('Firmware uploaded successfully!', 'green');
                otaStatusDiv.textContent = 'OTA Firmware Update Complete. Rebooting...';
                otaRebootTimer();
            } else {
                showStatusMessage('Firmware upload failed!', 'red');
                uploadButton.disabled = false;
                otaStatusDiv.textContent = '!!! Upload Error !!!';
            }
        }
    };
    
    // Send the request
    xhr.send(formData);
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) {
    if (oEvent.lengthComputable) {
        const percentComplete = (oEvent.loaded / oEvent.total) * 100;
        document.getElementById('prg').style.width = percentComplete + '%';
    } else {
        // Unable to compute progress information since the total size is unknown
        console.log("Unable to compute progress information since the total size is unknown");
    }
}

/**
 * Posts the firmware update status.
 */
function getUpdateStatus() {
    return fetch('/OTAstatus', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            const firmwareVersion = data.compile_date + ' - ' + data.compile_time;
            document.getElementById('fw_version').textContent = firmwareVersion;
            
            // Check OTA status
            const otaStatusDiv = document.getElementById('ota_update_status');
            if (data.ota_update_status === 1) {
                otaStatusDiv.textContent = 'OTA Firmware Update Complete. Rebooting...';
                otaRebootTimer();
            } else if (data.ota_update_status === -1) {
                otaStatusDiv.textContent = '!!! Upload Error !!!';
            } else {
                otaStatusDiv.textContent = '';
            }
            
            return data;
        })
        .catch(error => {
            console.error('Error getting update status:', error);
            throw error;
        });
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() {   
    let seconds = 10;
    const otaStatusDiv = document.getElementById('ota_update_status');
    
    const interval = setInterval(function() {
        seconds--;
        otaStatusDiv.textContent = 'OTA Firmware Update Complete. Rebooting in ' + seconds + ' seconds...';
        
        if (seconds <= 0) {
            clearInterval(interval);
            otaStatusDiv.textContent = 'Rebooting...';
            
            // Reload page after a short delay
            setTimeout(function() {
                window.location.reload();
            }, 5000);
        }
    }, 1000);
}

/**
 * Clears the connection status interval
 */
function stopWifiConnectStatusInterval() {
    if (wifiConnectStatusInterval) {
        clearInterval(wifiConnectStatusInterval);
        wifiConnectStatusInterval = null;
        console.log("WiFi connection status interval stopped.");
    }
}

/**
 * Gets the WiFi connection status
 */
function getWifiConnectStatus() {
    return fetch('/wifiConnectStatus', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            // Check the connection status
            if (data.wifi_connect_status === 3) {
                console.log("WiFi Status Code:", data.wifi_connect_status);
                console.log("Connection successful!");
                
                stopWifiConnectStatusInterval();
                document.getElementById('wifi_connect_status').textContent = "Connected successfully!";
                document.getElementById('wifi_connect_status').style.color = "green";
                
                // Dispatch a connection change event
                window.dispatchEvent(new CustomEvent('esp32-connection-change', { 
                    detail: { connected: true } 
                }));
                
                getConnectInfo();
            } else if (data.wifi_connect_status === 1) {
                document.getElementById('wifi_connect_status').textContent = "Connecting...";
                document.getElementById('wifi_connect_status').style.color = "blue";
            } else if (data.wifi_connect_status === 2) {
                stopWifiConnectStatusInterval();
                document.getElementById('wifi_connect_status').textContent = "Connection failed. Please try again.";
                document.getElementById('wifi_connect_status').style.color = "red";
                
                // Dispatch a connection change event
                window.dispatchEvent(new CustomEvent('esp32-connection-change', { 
                    detail: { connected: false } 
                }));
            }
            
            return data;
        })
        .catch(error => {
            console.error('Error getting WiFi connect status:', error);
            document.getElementById('wifi_connect_status').textContent = "Error checking connection status";
            document.getElementById('wifi_connect_status').style.color = "red";
            throw error;
        });
}

/**
 * Starts the interval for checking the connection status
 */
function startWifiConnectStatusInterval() {
    // Clear previous interval if it exists
    stopWifiConnectStatusInterval();
    
    console.log("Connection request sent, starting status interval...");
    document.getElementById('wifi_connect_status').textContent = "Connecting...";
    document.getElementById('wifi_connect_status').style.color = "blue";
    
    // Set up new interval to check status every 2 seconds
    wifiConnectStatusInterval = setInterval(function() {
        console.log("Checking WiFi connection status...");
        getWifiConnectStatus();
    }, 2000);
}

/**
 * Connect WiFi function called using the SSID and password entered into the text fields
 */
function connectWifi() {
    const ssid = document.getElementById('connect_ssid').value;
    const password = document.getElementById('connect_pass').value;
    
    // Send POST request to connect
    fetch('/wifiConnect.json', {
	  method: 'POST',
	  headers: {
		'my-connect-ssid': ssid,
		'my-connect-pwd': password,
	  }
	})

    .then(() => {
        // Start checking connection status
        startWifiConnectStatusInterval();
    })
    .catch(error => {
        console.error('Error connecting to WiFi:', error);
        document.getElementById('wifi_connect_status').textContent = "Error sending connection request";
        document.getElementById('wifi_connect_status').style.color = "red";
    });
}

/**
 * Checks credentials on connect_wifi button click
 */
function checkCredentials() {
    const ssid = document.getElementById('connect_ssid').value;
    const password = document.getElementById('connect_pass').value;
    const errorElement = document.getElementById('wifi_connect_credentials_errors');
    
    errorElement.textContent = "";
    
    if (!ssid) {
        errorElement.textContent = "Please enter a network name";
        return;
    }
    
    if (ssid.length > 32) {
        errorElement.textContent = "Network name is too long (max 32 characters)";
        return;
    }
    
    if (password.length > 64) {
        errorElement.textContent = "Password is too long (max 64 characters)";
        return;
    }
    
    // If all checks pass, attempt to connect
    console.log("Attempting to connect to SSID: " + ssid);
    connectWifi();
}

/**
 * Shows the WiFi password if the box is checked
 */
function showPassword() {
    const passwordField = document.getElementById('connect_pass');
    const checkBox = document.getElementById('show_password');
    
    if (checkBox.checked) {
        passwordField.type = "text";
    } else {
        passwordField.type = "password";
    }
}

/**
 * Gets the connection information for displaying on the webpage
 */
function getConnectInfo() {
    return fetch('/wifiConnectInfo.json')
        .then(response => response.json())
        .then(data => {
            console.log("Connected to:", data.ap);
            
            // Display connection info
            if (data.ap) {
                document.getElementById('connected-ssid').textContent = data.ap;
                document.getElementById('connect_ip').textContent = data.ip;
                document.getElementById('connect_gw').textContent = data.gw;
                document.getElementById('connect_mask').textContent = data.netmask;

                document.getElementById('connect_pass').value = '';
                document.getElementById('show_password').checked = false;
                document.getElementById('connect_pass').type = 'password';

                
                // Dispatch a connection change event
                window.dispatchEvent(new CustomEvent('esp32-connection-change', { 
                    detail: { connected: true } 
                }));
            } else {
                // Dispatch a connection change event
                window.dispatchEvent(new CustomEvent('esp32-connection-change', { 
                    detail: { connected: false } 
                }));
            }
            
            return data;
        })
        .catch(error => {
            console.error('Error getting connection info:', error);
            throw error;
        });
}

/**
 * Disconnects Wifi once the disconnect button is pressed and reloads the webpage
 */
function disconnectWifi() {
    fetch('/wifiDisconnect.json', { method: 'DELETE' })
        .then(() => {
            showStatusMessage("Disconnected from WiFi", "blue");
            
            // Dispatch a connection change event
            window.dispatchEvent(new CustomEvent('esp32-connection-change', { 
                detail: { connected: false } 
            }));
            
            // Reload the page after a short delay
            setTimeout(function() {
                window.location.reload();
            }, 1000);
        })
        .catch(error => {
            console.error('Error disconnecting from WiFi:', error);
            showStatusMessage("Error disconnecting from WiFi", "red");
        });
}

/**
 * Sets the interval for displaying local time
 */
function startLocalTimeInterval() {
    // Get initial time
    getLocalTime();
    
    // Update time every second
    networkIntervalId = setInterval(getLocalTime, 1000);
}

/**
 * Gets the local time
 * @note connect the ESP32 to the internet and the time will be updated
 */
function getLocalTime() {
    fetch('/localTime.json')
        .then(response => response.text())
        .then(data => {
            document.getElementById('local_time').textContent = data;
            return data;
        })
        .catch(error => {
            console.error('Error getting local time:', error);
            document.getElementById('local_time').textContent = '--:--:--';
        });
}