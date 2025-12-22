async function loadUsers() {
  let token = localStorage.getItem("token");
  if (token == null) {console.log("not logged in"); window.location.href = "/login.html"; return;}

  const response = await fetch('/api/lsusrs', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ' + token,
    }
  });
  if (response.status == 401) {
    localStorage.removeItem('token');
    window.location.href = '/login.html';
    return;
  }

  if (response.ok) {
    const data = await response.json();
    console.log(data);
    for (const u of data.users) {
      let div = document.createElement("div");
      div.className = "username-txt";
      let txt = document.createElement("p");
      txt.innerHTML = u;
      div.appendChild(txt);

      let delbtn = document.createElement("button");
      delbtn.innerHTML = "Delete User";
      delbtn.className = "user-del-btn";
      delbtn.addEventListener("click", (async () => {
	const res = await fetch("/api/a_delusr", {
	  method: "POST",
	  headers: {
	    'Content-Type': 'application/json',
	    'Authorization': 'Bearer ' + token,
	  },
	  body: JSON.stringify({
	    'uname': u
	  }),
	});
	if (res.ok) {
	  div.remove();
	}
	if (res.status == 401) {
	  localStorage.removeItem('token');
	  window.location.href = '/login.html';
	  return;
	}
	if (res.status == 404) {
	  console.log(res);
	  const data = await res.json();
	  document.getElementById("status").innerHTML = data.error;
	  loadUsers();
	}
	if (res.status == 400) {
	  const data = await res.json();
	  document.getElementById("status").innerHTML = data.error;
	}
      })
      );
      div.appendChild(delbtn);
      
      document.body.appendChild(div);
  }
  }

};

loadUsers();
