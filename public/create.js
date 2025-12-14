async function create() {
  let username = document.getElementById("username-input").value;
  let password = document.getElementById("password-input").value;
  let status = document.getElementById("status");

  const response = await fetch('/api/createusr',
    {
      method: 'POST',
      headers: {
	'Content-Type': 'application/json',
    },
      body: JSON.stringify({username: username, password: password})
    }
  );

  if (response.ok) {
    alert("User created. Redirecting to login.");
    window.location.href = '/login.html';
  }

  if (response.status == 409) {
    status.innerHTML = "User exists. Please pick a different username";
  }

  if (response.status == 400) {
    status.innerHTML = "An error has occurred. Please try again later.";
  }
};
