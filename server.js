
const express = require('express');
const cors = require('cors');
const admin = require('firebase-admin');

const serviceAccount = require('./firebase-key.json'); 
const PORT = 3000; 

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: 'YOUR_DATABASE_URL'
});
const db = admin.database();
const alertsRef = db.ref('alerts');
const app = express();
app.use(cors());
app.use(express.json()); 

let systemStatus = 'DISARMED'; 

app.post('/api/alert', (req, res) => {
  const alertData = req.body;
  console.log(`[${new Date().toISOString()}] Received alert:`, JSON.stringify(alertData));
  alertData.serverTimestamp = Date.now();
  alertsRef.push(alertData)
    .then(() => {
      console.log('Successfully saved alert to Firebase.');
      res.status(200).send({ message: 'Alert received and saved successfully.' });
    })
    .catch((error) => {
      console.error('Error saving alert to Firebase:', error);
      res.status(500).send({ message: 'Failed to save alert.' });
    });
});
app.get('/api/getstatus', (req, res) => {
  console.log(`[${new Date().toISOString()}] Status requested. Sending: ${systemStatus}`);
  res.status(200).json({ status: systemStatus });
});


app.post('/api/setstatus', (req, res) => {
  const { status } = req.body; 

  if (status === 'ARMED' || status === 'DISARMED') {
    systemStatus = status;
    console.log(`\n================================\nSYSTEM STATUS SET TO: ${systemStatus}\n================================\n`);
    res.status(200).json({ message: `System is now ${systemStatus}` });
  } else {
    res.status(400).json({ message: 'Invalid status. Please send "ARMED" or "DISARMED".' });
  }
});

app.get('/', (req, res) => {
    res.send(`
        <html style="font-family: sans-serif; background-color: #121b2e; color: white; text-align: center; padding-top: 50px;">
            <h1>PedalGuard Local Server</h1>
            <p>Current Status: <strong>${systemStatus}</strong></p>
            <p>Server is running and listening for alerts from the device.</p>
            <div style="margin-top: 40px;">
                <button onclick="setStatus('ARMED')" style="padding: 15px 30px; font-size: 16px; cursor: pointer; background-color: #e74c3c; color: white; border: none; border-radius: 5px; margin: 10px;">ARM SYSTEM</button>
                <button onclick="setStatus('DISARMED')" style="padding: 15px 30px; font-size: 16px; cursor: pointer; background-color: #2ecc71; color: white; border: none; border-radius: 5px; margin: 10px;">DISARM SYSTEM</button>
            </div>
            <script>
                function setStatus(newStatus) {
                    fetch('/api/setstatus', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ status: newStatus })
                    }).then(() => window.location.reload());
                }
            </script>
        </html>
    `);
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`PedalGuard local server listening on port ${PORT}`);
  console.log('Waiting for alerts from your device...');
  console.log('You can now arm/disarm the system by sending a POST request to /api/setstatus');
});
