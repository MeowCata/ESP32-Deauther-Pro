#include <WebServer.h>
#include "web_interface.h"
#include "definitions.h"
#include "deauth.h"
#include "esp_adc_cal.h"

WebServer server(80);
int num_networks;

float readTemperature() {
  return temperatureRead();
}

String getEncryptionType(wifi_auth_mode_t encryptionType);

void redirect_root() {
  server.sendHeader("Location", "/");
  server.send(301);
}

void handle_root() {
  float temperature = readTemperature(); 

  String html = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-Deauther</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                line-height: 1.6;
                color: #333;
                max-width: 800px;
                margin: 0 auto;
                padding: 20px;
                background-color: #f4f4f4;
            }
            #networkTable tr:hover {
                background-color: #e6f2ff !important;
            }
        h1, h2 {
            color: #2c3e50;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-bottom: 20px;
        }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background-color: #3498db;
            color: white;
        }
        tr:nth-child(even) {
            background-color: #f2f2f2;
        }
        form {
            background-color: white;
            padding: 20px;
            border-radius: 5px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        input[type="text"], input[type="number"], input[type="submit"] {
            width: 100%;
            padding: 10px;
            margin-bottom: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        .button-group {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        input[type="submit"] {
            background-color: #3498db;
            color: white;
            border: none;
            cursor: pointer;
            transition: all 0.3s;
            padding: 12px;
            border-radius: 4px;
            font-weight: bold;
        }
        input[type="submit"]:hover {
            background-color: #2980b9;
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);
        }
        .danger {
            background-color: #e74c3c;
        }
        .danger:hover {
            background-color: #c0392b;
        }
        .success {
            background-color: #2ecc71;
        }
        .success:hover {
            background-color: #27ae60;
        }
        .warning {
            background-color: #f39c12;
        }
        .warning:hover {
            background-color: #d35400;
        }
        .button-icon {
            margin-right: 8px;
        }
    </style>
</head>
<body>
    <h1>Wireless Deauth</h1>
    <h2>ChipTemp: )raw" + String(temperature, 1) + R"raw( °C</h2>

    <h2>WiFi Networks</h2>
    <table id="networkTable">
        <tr>
            <th>Number</th>
            <th>SSID</th>
            <th>BSSID</th>
            <th>Channel</th>
            <th>Range</th>
            <th>Encryption</th>
        </tr>)raw";

  for (int i = 0; i < num_networks; i++) {
    String encryption = getEncryptionType(WiFi.encryptionType(i));
    html += "<tr><td>" + String(i) + "</td><td>" + WiFi.SSID(i) + "</td><td>" + WiFi.BSSIDstr(i) + "</td><td>" + 
            String(WiFi.channel(i)) + "</td><td>" + String(WiFi.RSSI(i)) + "</td><td>" + encryption + "</td></tr>";
  }

  html += R"raw(
    </table>

    <div class="button-group">
        <form method="post" action="/rescan">
            <input type="submit" value="🔍 Rescan networks" class="success">
        </form>
        <form method="post" action="/stop">
            <input type="submit" value="⏹ Stop Deauth" class="warning">
        </form>
        <form method="post" action="/stop_ap">
            <input type="submit" value="📶 Stop AP" class="warning">
        </form>
    </div>

    <div class="button-group">
                <form method="post" action="/deauth" id="deauthForm">
                    <h2>Launch Deauth-Attack</h2>
                    <p>Click on a network row above to select target</p>
                    <div id="selectedNetworkDisplay" style="margin: 10px 0; padding: 10px; background: #f0f0f0; border-radius: 4px; display: none;">
                        Selected Network: <span id="selectedNetworkInfo">None</span>
                    </div>
                    <input type="hidden" name="net_num" id="selectedNetwork">
                    <input type="number" name="reason" placeholder="Reason Code" min='0' max='24' value='1'>
                    <input type="submit" value="⚡ Launch Attack" class="warning">
                </form>
    </div>
    <script> //by ds-v3
        document.addEventListener('DOMContentLoaded', function() {
            const table = document.getElementById('networkTable');
            const rows = table.getElementsByTagName('tr');
            const form = document.getElementById('deauthForm');
            const selectedNetworkInput = document.getElementById('selectedNetwork');
            
            for (let i = 1; i < rows.length; i++) {
                rows[i].style.cursor = 'pointer';
                rows[i].addEventListener('click', function() {
                    // Remove highlight from all rows
                    for (let j = 1; j < rows.length; j++) {
                        rows[j].style.backgroundColor = '';
                    }
                    // Highlight selected row (removed blue border and bold)
                    this.style.backgroundColor = '#d4edff';
                    // Get network number from first cell
                    selectedNetworkInput.value = this.cells[0].textContent;
                    // Update display
                    document.getElementById('selectedNetworkDisplay').style.display = 'block';
                    document.getElementById('selectedNetworkInfo').textContent = 
                        '#' + this.cells[0].textContent + ' - ' + this.cells[1].textContent + 
                        ' (BSSID: ' + this.cells[2].textContent + ')';
                });
            }
        });
    </script>

    <h3>Eliminated devices: )raw" + String(eliminated_stations) + R"raw(</h3>

    <div class="button-group">
        <form method="post" action="/deauth_all">
            <input type="number" name="reason" placeholder="Reason Code" min='0' max='24' value='1'>  
            <input type="submit" value="☠ DEAUTH ALL" class="danger">
        </form>
    </div>
    <h2>Reason Codes</h2>
    <table>
        <tr>
            <th>Code</th>
            <th>Meaning</th>
        </tr>
        <tr><td>0</td><td>Reserved.</td></tr>
        <tr><td>1</td><td>Unspecified reason.</td></tr>
        <tr><td>2</td><td>Previous authentication no longer valid.</td></tr>
        <tr><td>3</td><td>Deauthenticated because sending station (STA) is leaving or has left Independent Basic Service Set (IBSS) or ESS.</td></tr>
        <tr><td>4</td><td>Disassociated due to inactivity.</td></tr>
        <tr><td>5</td><td>Disassociated because WAP device is unable to handle all currently associated STAs.</td></tr>
        <tr><td>6</td><td>Class 2 frame received from nonauthenticated STA.</td></tr>
        <tr><td>7</td><td>Class 3 frame received from nonassociated STA.</td></tr>
        <tr><td>8</td><td>Disassociated because sending STA is leaving or has left Basic Service Set (BSS).</td></tr>
        <tr><td>9</td><td>STA requesting (re)association is not authenticated with responding STA.</td></tr>
        <tr><td>10</td><td>Disassociated because the information in the Power Capability element is unacceptable.</td></tr>
        <tr><td>11</td><td>Disassociated because the information in the Supported Channels element is unacceptable.</td></tr>
        <tr><td>12</td><td>Disassociated due to BSS Transition Management.</td></tr>
        <tr><td>13</td><td>Invalid element, that is, an element defined in this standard for which the content does not meet the specifications in Clause 8.</td></tr>
        <tr><td>14</td><td>Message integrity code (MIC) failure.</td></tr>
        <tr><td>15</td><td>4-Way Handshake timeout.</td></tr>
        <tr><td>16</td><td>Group Key Handshake timeout.</td></tr>
        <tr><td>17</td><td>Element in 4-Way Handshake different from (Re)Association Request/ Probe Response/Beacon frame.</td></tr>
        <tr><td>18</td><td>Invalid group cipher.</td></tr>
        <tr><td>19</td><td>Invalid pairwise cipher.</td></tr>
        <tr><td>20</td><td>Invalid AKMP.</td></tr>
        <tr><td>21</td><td>Unsupported RSNE version.</td></tr>
        <tr><td>22</td><td>Invalid RSNE capabilities.</td></tr>
        <tr><td>23</td><td>IEEE 802.1X authentication failed.</td></tr>
        <tr><td>24</td><td>Cipher suite rejected because of the security policy.</td></tr>
    </table>
</body>
</html>)raw";

  server.send(200, "text/html", html);
}

void handle_deauth() {
  int wifi_number = server.arg("net_num").toInt();
  uint16_t reason = server.arg("reason").toInt();

  String html = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-Deauther</title>
    <style>
        body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; max-width: 800px; margin: 0 auto; padding: 20px; }
        .alert { background-color: #f4f4f4; padding: 20px; border-radius: 5px; }
        .error { background-color: #ffe6e6; }
        .button { display: inline-block; padding: 10px 20px; background-color: #3498db; color: white; text-decoration: none; border-radius: 4px; }
        .button:hover { background-color: #2980b9; }
    </style>
</head>
<body>
    <div class="alert)raw";

  if (wifi_number < num_networks) {
    html += R"raw(">
        <h2>Starting Deauth-Attack!</h2>
        <p>Deauthenticating network number: )raw" + String(wifi_number) + R"raw(</p>
        <p>Reason code: )raw" + String(reason) + R"raw(</p>
    </div>)raw";
    start_deauth(wifi_number, DEAUTH_TYPE_SINGLE, reason);
  } else {
    html += R"raw( error">
        <h2>Error: Invalid Network Number</h2>
        <p>Please select a valid network number.</p>
    </div>)raw";
  }

  html += R"raw(
    <a href="/" class="button">Back to Home</a>
</body>
</html>)raw";

  server.send(200, "text/html", html);
}

void handle_deauth_all() {
  uint16_t reason = server.arg("reason").toInt();

  String html = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-Deauther</title>
    <style>
        body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; max-width: 800px; margin: 0 auto; padding: 20px; }
        .alert { background-color: #f4f4f4; padding: 20px; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="alert">
        <h2>Starting Deauth-Attack on All Networks!</h2>
        <p>WiFi will shut down now. To stop the attack, please reset the ESP32.</p>
        <p>Reason code: )raw" + String(reason) + R"raw(</p>
    </div>
</body>
</html>)raw";

  server.send(200, "text/html", html);
  server.stop();
  start_deauth(0, DEAUTH_TYPE_ALL, reason);
}

void handle_rescan() {
  num_networks = WiFi.scanNetworks(false, true, true);
  redirect_root();
}

void handle_stop() {
  stop_deauth();
  redirect_root();
}

void handle_stop_ap() {
  WiFi.softAPdisconnect(true);
  redirect_root();
}

void start_web_interface() {
  server.on("/", handle_root);
  server.on("/deauth", handle_deauth);
  server.on("/deauth_all", handle_deauth_all);
  server.on("/rescan", handle_rescan);
  server.on("/stop", handle_stop);
  server.on("/stop_ap", handle_stop_ap);
  handle_rescan();
  server.begin();
}

void web_interface_handle_client() {
  server.handleClient();
}

String getEncryptionType(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN:
      return "Open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2_ENTERPRISE";
    default:
      return "UNKNOWN";
  }
}
