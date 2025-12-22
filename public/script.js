async function refresh() {
  let token = localStorage.getItem("token");
  if (token == null) {console.log("not logged in"); window.location.href = "/login.html"; return;}

  let msgslist = document.getElementById("msgslist");
  msgslist.innerHTML = "";


  const response = await fetch('/api/getmsgs', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ' + token,
    },
  });
  if (response.status == 401) {
    localStorage.removeItem('token');
    window.location.href = '/login.html';
    return;
  }

  else if (!response.ok) {
    throw new Error("Failed to load messages.");
  }

  const data = await response.json();

  console.log(data);

  if (response.ok) {
    for (const message of data.messages) {
      let div = document.createElement("div");
      div.className = "msg-div"
      let txt = document.createElement("p");
      txt.innerHTML = `(${message.from}) Subject: ${message.subject}`;
      txt.style.display = "inline-block";
      txt.className = "msg-title";

      let btn = document.createElement("button");
      btn.innerHTML = "Hide";
      btn.className = "view-msg-button";

      let btn2 = document.createElement("button");
      btn2.innerHTML = "Delete";
      btn2.className = "delete-msg-button";

      let body_txt_div = document.createElement("div");
      let body_txt = document.createElement("p");
      body_txt.innerHTML = message.body;
      body_txt_div.appendChild(body_txt);
      body_txt_div.style.display = "block";

      btn.addEventListener("click", (async () => {
	if (body_txt_div.style.display === "none") {
	  body_txt_div.style.display = "block";
	  btn.innerHTML = "Hide";
	}
	else {
	  body_txt_div.style.display = "none";
	  btn.innerHTML = "View";
	}
      }
      ));

      btn2.addEventListener("click", (async () => {
	const res = await fetch("/api/delmsg", {
	  method: "POST",
	  headers: {
	    'Content-Type': 'application/json',
	    'Authorization': 'Bearer ' + token,
	  },
	  body: JSON.stringify({
	    'id': message.id,
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
      }));

      body_txt_div.className = "msg-body";
      div.appendChild(txt);
      div.appendChild(btn);
      div.appendChild(btn2);
      div.appendChild(body_txt_div);
      msgslist.appendChild(div);
    };
  }
}

async function logout() {
  let token = localStorage.getItem("token");
  if (token == null){window.location.href = "/login.html"; return;}

  const response = await fetch('/api/logout', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ' + token,
  }});

  localStorage.removeItem('token');
  window.location.href = '/login.html';
}

async function send_message() {
  let token = localStorage.getItem("token");
  if (token == null){window.location.href = "/login.html"; return;}

  let to = document.getElementById("input-msg-to");
  let subject = document.getElementById("input-msg-subject");
  let body_input = document.getElementById("input-msg-body");
  let status = document.getElementById("status");

  if (to.value.trim() == "") {
    status.innerHTML = "Please fill out the fields.";
    return;
  }

  const response = await fetch('/api/createmsg', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ' + token,
  },
    body: JSON.stringify({
      'to': to.value,
      'subject': subject.value,
      'body': body_input.value
    })
  });

  const data = await response.json();

  if (response.status == 200) {
    status.innerHTML = "Message sent."
    to.value = "";
    subject.value = "";
    body_input.value = "";
  }

  if (response.status == 401) {
    localStorage.removeItem("token");
    window.location.href = "/login.html";
  }

  if (response.status == 404 || response.status == 400) {
    console.log(data);
    status.innerHTML = data[1];
  }

};

async function delusr() {
  let token = localStorage.getItem("token");
  if (token == null){window.location.href = "/login.html"; return;}

  const response = await fetch('/api/delusr', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ' + token,
  },
  });

  const data = await response.json();

  if (response.status == 401 || response.status == 200) {
    localStorage.removeItem("token");
    window.location.href = "/login.html";
  }

  if (response.status == 404 || response.status == 400) {
    console.log(data);
    status.innerHTML = data[1];
  }
};

refresh();

setInterval(refresh, 10000);
