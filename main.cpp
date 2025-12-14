#include "httplib.h"
#include "json.hpp"
#include "sqlite3.h"
#include <algorithm>
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

class Database {
private:
  sqlite3 *db;

public:
  Database(const std::string &db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc) {
      std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
      throw std::runtime_error("Failed to open database");
    }
    initTables();
  }

  ~Database() { sqlite3_close(db); }

  void initTables() {
    const char *sql = R"(
	create table if not exists users (
	id integer primary key autoincrement,
	username text unique not null,
	password text not null
	);

	create table if not exists messages (
	  id text primary key,
	  from_user text not null,
	  to_user text not null,
	  subject text not null,
	  body text not null,
	  created_at timestamp default current_timestamp
	  );

	  create index if not exists idx_messages_to on messages(to_user);
	  create index if not exists idx_messages_from on messages(from_user);
      )";

    char *errMsg;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
      std::cerr << "SQL error: " << errMsg << std::endl;
      sqlite3_free(errMsg);
    }
  }

  bool createUser(const std::string &username, const std::string &password) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO users (username, password) VALUES (?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
      return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
  }

  bool verifyUser(const std::string &username, const std::string &password) {
    sqlite3_stmt *stmt;
    const char *sql = "select password from users where username = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
      return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool verified = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *stored_password =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      verified = (password == stored_password);
    }

    sqlite3_finalize(stmt);
    return verified;
  }

  bool userExists(const std::string &username) {
    sqlite3_stmt *stmt;
    const char *sql = "select 1 from users where username = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
      return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
  }

  bool createMessage(const Message &msg) {
    sqlite3_stmt *stmt;
    const char *sql = "insert into messages (id, from_user, to_user, subject, "
                      "body) VALUES (?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
      return false;
    }

    sqlite3_bind_text(stmt, 1, msg.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, msg.from.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, msg.to.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, msg.subject.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, msg.body.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
  }

  std::vector<Message> getMessagesForUser(const std::string &username) {
    std::vector<Message> messages;
    sqlite3_stmt *stmt;
    const char *sql = "select id, from_user, to_user, subject, body from "
                      "messages where to_user = ? order by created_at desc";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
      return messages;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string id =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      std::string from =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      std::string to =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
      std::string subject =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
      std::string body =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));

      messages.emplace_back(from, to, subject, body, id);
    }

    sqlite3_finalize(stmt);
    return messages;
  }

  bool deleteMessage(const std::string &username, const std::string &msg_id) {
    sqlite3_stmt *stmt;
    const char *sql = "delete from messages where id = ? and to_user = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
      return false;
    }

    sqlite3_bind_text(stmt, 1, msg_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE && sqlite3_changes(db) > 0;
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
  Database db("messages.db");

  std::map<std::string, std::string> sessions;

  httplib::Server svr;

  svr.set_mount_point("/", "./public");

  svr.Post("/api/login", [&db, &sessions](const auto &req, auto &res) {
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

    if (!db.userExists(uname)) {
      res.status = 404;
      res.set_content(R"({"error": "user not found")", "application/json");
      return;
    }

    if (db.verifyUser(uname, password)) {
      res.status = 200;
      std::string token = generateToken();
      sessions[token] = uname;

      json response = {
          {"success", true}, {"token", token}, {"username", uname}};

      res.set_content(response.dump(), "application/json");
    } else {
      res.status = 401;
      json error_str = {{"error", "incorrect password"}};
      res.set_content(error_str.dump(), "application/json");
    }
  });

  svr.Post("/api/logout", [&db, &sessions](const auto &req, auto &res) {
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

  svr.Post("/api/createusr", [&db](const auto &req, auto &res) {
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

    if (db.userExists(uname)) {
      res.status = 409;
      json response = {"error", "user exists."};
      res.set_content(response.dump(), "application/json");
      return;
    }

    db.createUser(uname, passwd);

    res.status = 200;
  });

  svr.Post("/api/getmsgs", [&db, &sessions](const auto &req, auto &res) {
    std::string auth = req.get_header_value("Authorization");

    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
      res.status = 401;
      return;
    }

    std::string token = auth.substr(7);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
      res.status = 401;
      json msg = {"error", "session expired"};
      res.set_content(msg.dump(), "application/json");
      return;
    }

    std::string username = it->second;

    if (db.userExists(username)) {
      res.status = 200;
      std::vector<Message> msgs = db.getMessagesForUser(username);
      json msg_array = json::array();
      for (const auto &m : msgs)
        msg_array.push_back(m);
      json response = {{"messages", msg_array}};
      res.set_content(response.dump(), "application/json");
      return;
    }
  });

  svr.Post("/api/createmsg", [&db, &sessions](const auto &req, auto &res) {
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
    std::string to, subject, body;

    try {
      std::cout << req.body << std::endl;
      auto data = json::parse(req.body);
      to = data["to"];
      subject = data["subject"];
      body = data["body"];
    } catch (json::parse_error &e) {
      res.status = 400;
      json error = {{"error", "failed to parse JSON"}};
      res.set_content(error.dump(), "application/json");
      return;
    }

    if (!db.userExists(to)) {
      res.status = 404;
      json error = {"error", "recipient does not exist"};
      res.set_content(error.dump(), "application/json");
      return;
    }

    Message msg(username, to, subject, body, generateToken());
    if (db.createMessage(msg)) {
      res.status = 200;
      res.set_content(R"({"status": "message sent."})", "application/json");
    } else {
      res.status = 500;
      json error = {{"error", "failed to create message"}};
      res.set_content(error.dump(), "application/json");
    }
  });

  svr.Post("/api/delmsg", [&db, &sessions](const auto &req, auto &res) {
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

    if (db.deleteMessage(username, id)) {
      res.status = 200;
      res.set_content("{\"status\": \"Success\"}", "application/json");
      return;
    } else {
      res.status = 404;
      json error = {{"error", "message not found"}};
      res.set_content(error.dump(), "application/json");
      return;
    }
  });

  std::cout << "Server running on http://localhost:8080\n";
  svr.listen("0.0.0.0", 8080);
}
