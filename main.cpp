#include "httplib.h"
#include "json.hpp"
#include <iostream>
#include <string>

using json = nlohmann::json;

struct Message {
public:
  std::string from;
  std::string to;
  std::string subject;
  std::string body;
  std::string id;

  Message(std::string from, std::string to, std::string subject,
          std::string body, std::string id)
      : from(from), to(to), subject(subject), body(body), id(id) {};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Message, from, to, subject, body, id);

struct User {
  std::string username = "";
  std::string password = "";
  std::vector<Message> msgs = {};

  User(std::string username, std::string password) {
    this->username = username;
    this->password = password;
  }
};

std::string generateToken() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  std::stringstream ss;
  for (int i = 0; i < 32; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
  }
  return ss.str();
}

int main() {
  std::vector<User> users = {User("fish", "123"), User("jeef", "123")};

  std::map<std::string, std::string> sessions; // token, username

  httplib::Server svr;

  svr.set_mount_point("/", "./public");

  svr.Post("/api/login", [&users, &sessions](const auto &req, auto &res) {
    std::cout << " Received request: " << req.body << '\n';

    std::string uname;
    std::string password;

    try {
      auto j = json::parse(req.body);
      uname = j["username"];
      password = j["password"];
    } catch (json::parse_error &e) {
      res.status = 400;
      json error = {"error", "could not parse JSON"};
      res.set_content(error.dump(), "application/json");
      return;
    };

    for (User usr : users) {
      std::cout << usr.username << std::endl;
      if (usr.username == uname) {
        if (usr.password == password) {
          res.status = 200;

          std::string token = generateToken();
          sessions[token] = usr.username;

          json response = {
              {"success", true}, {"token", token}, {"username", usr.username}};

          res.set_content(response.dump(), "application/json");

          return;
        } else {
          res.status = 401;
          json error_str = {"error", "incorrect password"};
          res.set_content(error_str.dump(), "application/json");
          return;
        }
      }
    }

    res.status = 404;
    json u_not_found = {"error", "username not found"};
    res.set_content(u_not_found.dump(), "application/json");
  });

  svr.Post("/api/logout", [&users, &sessions](const auto &req, auto &res) {
    std::cout << "Received logout request.\n";
    std::string auth = req.get_header_value("Authorization");
    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
      res.status = 401;
      std::cout << "bad user auth\n";
      return;
    }

    std::string token = auth.substr(7);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
      res.status = 401;
      json msg = {"error", "session expired"};
      res.set_content(msg.dump(), "application/json");
      std::cout << "bad user auth\n";
      return;
    }

    sessions.erase(it);
    res.status = 200;
    return;
  });

  svr.Post("/api/createusr", [&users](const auto &req, auto &res) {
    std::cout << "Creating new user.\n";

    std::string uname;
    std::string passwd;

    try {
      auto body = json::parse(req.body);
      uname = body["username"];
      passwd = body["password"];
    } catch (json::parse_error &e) {
      res.status = 400;
      json error = {"error", "could not parse JSON"};
      res.set_content(error.dump(), "application/json");
      return;
    }

    for (User &usr : users) {
      if (usr.username == uname) {
        res.status = 409;
        json response = {"error", "user exists."};
        res.set_content(response.dump(), "application/json");
        return;
      }
    }

    users.push_back(User(uname, passwd));
    std::cout << "Added user " << uname << " with password " << passwd << '\n';

    res.status = 200;
  });

  svr.Post("/api/getmsgs", [&users, &sessions](const auto &req, auto &res) {
    std::string auth = req.get_header_value("Authorization");

    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
      res.status = 401;
      std::cout << "bad user auth\n";
      return;
    }

    std::string token = auth.substr(7);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
      res.status = 401;
      json msg = {"error", "session expired"};
      res.set_content(msg.dump(), "application/json");
      std::cout << "bad user auth\n";
      return;
    }

    std::string username = it->second;

    for (User &u : users) {
      if (u.username == username) {
        res.status = 200;
        std::vector<std::string> msg_strs;
        for (Message &m : u.msgs) {
          json mj = m;
          msg_strs.push_back(mj.dump());
        }
        json response = {{"messages", msg_strs}};
        res.set_content(response.dump(), "application/json");
        return;
      }
    }

    res.status = 404;
    std::cout << "bad user auth\n";
    return;
  });

  svr.Post("/api/createmsg", [&users, &sessions](const auto &req, auto &res) {
    std::string auth = req.get_header_value("Authorization");

    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
      res.status = 401;
      std::cout << "bad user auth\n";
      return;
    }

    std::string token = auth.substr(7);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
      res.status = 401;
      json msg = {"error", "session expired"};
      res.set_content(msg.dump(), "application/json");
      std::cout << "bad user auth\n";
      return;
    }

    std::string username = it->second;

    for (User &u : users) {
      if (u.username == username) {
        std::string to, from, subject, body;
        from = u.username;
        try {
          std::cout << req.body << std::endl;
          auto data = json::parse(req.body);
          to = data["to"];
          subject = data["subject"];
          body = data["body"];
        } catch (json::parse_error &e) {
          res.status = 400;
          json error = {"error", "failed to parse JSON"};
          res.set_content(error.dump(), "application/json");
          return;
        }

        for (User &ut : users) {
          if (ut.username == to) {
            res.status = 200;
            ut.msgs.push_back(
                Message(from, to, subject, body, generateToken()));
            res.set_content("{}", "application/json");
            return;
          }
        }

        res.status = 404;
        json error = {"error", "recipient does not exist"};
        res.set_content(error.dump(), "application/json");

        return;
      }
    }

    res.status = 404;
    json error = {"error", "could not find user"};
    res.set_content(error.dump(), "application/json");
  });

  svr.Post("/api/delmsg", [&users, &sessions](const auto &req, auto &res) {
    std::string auth = req.get_header_value("Authorization");

    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
      res.status = 401;
      std::cout << "bad user auth\n";
      return;
    }

    std::string token = auth.substr(7);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
      res.status = 401;
      json msg = {"error", "session expired"};
      res.set_content(msg.dump(), "application/json");
      std::cout << "bad user auth\n";
      return;
    }
    std::string id;
    try {
      std::cout << req.body << std::endl;
      auto data = json::parse(req.body);
      id = data["id"];
    } catch (json::parse_error &e) {
      res.status = 400;
      json error = {"error", "failed to parse JSON"};
      res.set_content(error.dump(), "application/json");
      return;
    }

    std::string username = it->second;

    for (User &u : users) {
      if (u.username == username) {
        for (auto it = u.msgs.begin(); it != u.msgs.end(); it++) {
          if (it->id == id) {
            res.status = 200;
            res.set_content("{\"status\": \"Success\"}", "application/json");
            u.msgs.erase(it);
            return;
          }
        }

        return;
      }
    }

    res.status = 404;
    json error = {"error", "could not find user"};
    res.set_content(error.dump(), "application/json");
  });

  std::cout << "Server running on http://localhost:8080\n";
  svr.listen("0.0.0.0", 8080);
}
