// Funções da página de login e gerenciamento
window.onload = function() {
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.has('error') && urlParams.get('error') === '1') {
    const errorMessageElement = document.querySelector('.error');
    if (errorMessageElement) {
      errorMessageElement.innerText = 'Invalid Username or Password';
    }
  }
  if (document.getElementById('userList')) {
    getUsers();
  }
  if (document.getElementById('uidStatus')) {
    startUidPolling();
  }
};

function openDoor() {
  fetch('/openDoor', {
      method: 'POST'
    })
    .then(response => response.text())
    .then(data => {
      alert(data);
    })
    .catch(error => {
      console.error('Error (openDoor):', error);
      alert('Error opening door. Check console for details.');
    });
}

function getUsers() {
  fetch('/getUsers')
    .then(response => response.json())
    .then(data => {
      const userList = document.getElementById('userList');
      userList.innerHTML = '<tr><th>RA</th><th>Name</th><th>UID</th><th>Action</th></tr>';
      if (data && data.users && Array.isArray(data.users)) {
        data.users.forEach(user => {
          const row = userList.insertRow();
          row.insertCell().innerText = user.ra;
          row.insertCell().innerText = user.name;
          row.insertCell().innerText = user.uid;
          const actionCell = row.insertCell();
          const removeBtn = document.createElement('button');
          removeBtn.innerText = 'Remove';
          removeBtn.className = 'remove-btn';
          removeBtn.onclick = () => removeUser(user.ra);
          actionCell.appendChild(removeBtn);
        });
      } else {
        userList.innerHTML = '<tr><td colspan="4">No users registered or error in data.</td></tr>';
      }
    })
    .catch(error => {
      console.error('Error fetching users:', error);
      document.getElementById('userList').innerHTML = '<tr><td colspan="4">Error loading users. Check console.</td></tr>';
    });
}

function removeUser(ra) {
  if (confirm('Are you sure you want to remove user ' + ra + '?')) {
    fetch('/removeUser', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: 'ra=' + ra
      })
      .then(response => response.text())
      .then(data => {
        alert(data);
        getUsers();
      })
      .catch(error => {
        console.error('Error removing user:', error);
        alert('Error removing user. Check console for details.');
      });
  }
}

let uidPollingInterval;

function startUidPolling() {
  if (uidPollingInterval) clearInterval(uidPollingInterval);
  uidPollingInterval = setInterval(fetchLastScannedUid, 1000);
  document.getElementById('uidStatus').innerText = 'Awaiting RFID scan...';
  document.getElementById('uidInput').value = '';
}

function stopUidPolling() {
  if (uidPollingInterval) clearInterval(uidPollingInterval);
  uidPollingInterval = null;
}

function fetchLastScannedUid() {
  fetch('/getLastScannedUid')
    .then(response => response.text())
    .then(uid => {
      if (uid && uid !== "No UID") {
        document.getElementById('uidInput').value = uid;
        document.getElementById('uidStatus').innerText = 'UID Scanned: ' + uid;
        stopUidPolling();
      }
    })
    .catch(error => {
      console.error('Error fetching UID:', error);
      document.getElementById('uidStatus').innerText = 'Error fetching UID. Check console.';
    });
}

function registerUser() {
  const ra = document.getElementById('raInput').value;
  const name = document.getElementById('nameInput').value;
  const uid = document.getElementById('uidInput').value;
  const messageElement = document.getElementById('message');

  if (!ra || !name || !uid) {
    if (messageElement) {
      messageElement.className = 'error';
      messageElement.innerText = 'All fields are required!';
    }
    return;
  }

  fetch('/registerUser', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: 'ra=' + ra + '&name=' + name + '&uid=' + uid
    })
    .then(response => response.text())
    .then(data => {
      if (messageElement) {
        if (data.includes('Error')) {
          messageElement.className = 'error';
        } else {
          messageElement.className = 'success';
          document.getElementById('raInput').value = '';
          document.getElementById('nameInput').value = '';
          document.getElementById('uidInput').value = '';
          startUidPolling(); // Restart polling for next user
        }
        messageElement.innerText = data;
      }
    })
    .catch(error => {
      console.error('Error registering user:', error);
      if (messageElement) {
        messageElement.className = 'error';
        messageElement.innerText = 'An error occurred during registration. Check console.';
      }
    });
}

window.onbeforeunload = stopUidPolling;