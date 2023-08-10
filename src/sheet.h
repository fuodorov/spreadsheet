#pragma once

#include <functional>
#include <vector>

#include "cell.h"
#include "common.h"

class Sheet : public SheetInterface {
 public:
  ~Sheet();

  void SetCell(Position position, std::string text) override;
  CellInterface* GetCell(Position position) override;
  const CellInterface* GetCell(Position position) const override;
  Cell* GetConcreteCell(Position position);
  const Cell* GetConcreteCell(Position position) const;
  void ClearCell(Position position) override;
  Size GetPrintableSize() const override;
  void PrintValues(std::ostream& output) const override;
  void PrintTexts(std::ostream& output) const override;

 private:
  std::vector<std::vector<std::unique_ptr<Cell>>> spreadsheet_;
};