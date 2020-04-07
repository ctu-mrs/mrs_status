#include <commons.h>

class Menu {

public:
  Menu(int begin_y, int begin_x, std::vector<std::string> &text);
  Menu(int begin_y, int begin_x, std::vector<std::string> &text, int id);

  WINDOW *getWin();
  int     getLine();
  int     getId();

  std::optional<std::tuple<int, int>> iterate(std::vector<std::string> &text, int key, bool refresh);

private:
  WINDOW *win_;
  int     line = 0;
  int     id_;
};
