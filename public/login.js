async function login() {
  let uname = document.getElementById("username-input").value;
  let password = document.getElementById("password-input").value;
  let status = document.getElementById("status");
  status.innerHTML = ' ';

  const response = await fetch('/api/login', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ username: uname, password: password })
  });


  if (response.ok) {
    const data = await response.json();
    localStorage.setItem("token", data.token);
    if (uname == "admin") {
      window.location.href = '/admin.html';
    } else {
      window.location.href = '/';
    }
    };
  if (response.status == 404) {
    status.innerHTML = "User not found.";
  }
  if (response.status == 401) {
    status.innerHTML = "Incorrect password.";
  }
}

