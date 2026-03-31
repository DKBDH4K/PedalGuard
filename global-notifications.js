
const firebaseConfig = {
  apiKey: "YOUR_API_KEY", 
  authDomain: "YOUR_AUTH_DOMAIN",
  databaseURL: "YOUR_DATABASE_URL",
  projectId: "YOUR_PROJECT_ID",
  storageBucket: "YOUR_STORAGE_BUCKET",
  messagingSenderId: "YOUR_MESSAGING_SENDER_ID",
  appId: "YOUR_APP_ID"
};


if (!firebase.apps.length) { 
  firebase.initializeApp(firebaseConfig); 
}

const database = firebase.database();
const alertsRef = database.ref('alerts');


const alertSound = new Audio("data:audio/wav;base64,UklGRl9vT19XQVZFZm10IBAAAAABAAEAQB8AAEAfAAABAAgAZGF0YU"+Array(30).join("120112112120"));

const scriptLoadTime = Date.now();
console.log('Global notifier ready. Listening for new alerts...');


alertsRef.orderByChild('serverTimestamp').startAt(scriptLoadTime).on('child_added', (snapshot) => {
    if (!snapshot.exists()) return;

    const alertData = snapshot.val();
    
    alertSound.play().catch(e => console.error("Sound play failed:", e));

    const message = `Alert: ${alertData.trigger || "Unknown"} Detected`;
    showNotificationToast(message);
});



function showNotificationToast(message) {
   
    const toast = document.createElement('div');
    toast.className = 'pedalguard-toast';
    
 
    const icon = document.createElement('span');
   
    icon.className = 'fas fa-exclamation-triangle pedalguard-toast-icon';

  
    const text = document.createElement('span');
    text.textContent = message;

    toast.appendChild(icon);
    toast.appendChild(text);

    if (!document.getElementById('pedalguard-toast-styles')) {
        const style = document.createElement('style');
        style.id = 'pedalguard-toast-styles';
      
        style.innerHTML = `
            .pedalguard-toast {
                position: fixed;
                top: 20px;
                right: 20px;
                background: rgba(31, 42, 58, 0.9); /* From your li bg */
                color: white;
                padding: 16px 24px;
                border-radius: 8px;
                border: 1px solid rgba(255, 255, 255, 0.15);
                backdrop-filter: blur(10px);
                -webkit-backdrop-filter: blur(10px);
                box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.3);
                font-family: 'Poppins', sans-serif;
                font-size: 1rem;
                display: flex;
                align-items: center;
                gap: 12px;
                z-index: 9999;
                transform: translateX(110%);
                animation: pedalGuardSlideIn 0.5s cubic-bezier(0.25, 0.8, 0.25, 1) forwards;
            }
            .pedalguard-toast-icon {
                font-size: 1.5rem;
                color: #f59e0b; /* A bright warning color */
            }
            @keyframes pedalGuardSlideIn {
                to { transform: translateX(0); }
            }
            @keyframes pedalGuardSlideOut {
                from { 
                    transform: translateX(0); 
                    opacity: 1;
                }
                to { 
                    transform: translateX(110%);
                    opacity: 0;
                }
            }
        `;
        document.head.appendChild(style);
    }


    document.body.appendChild(toast);

    
    setTimeout(() => {
        toast.style.animation = 'pedalGuardSlideOut 0.5s cubic-bezier(0.25, 0.8, 0.25, 1) forwards';
        toast.addEventListener('animationend', () => {
            if (toast.parentElement) {
                toast.parentElement.removeChild(toast);
            }
        });
    }, 5000);
}

