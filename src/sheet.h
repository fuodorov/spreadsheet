#pragma once

#include <functional>
#include <vector>

#include "cell.h"
#include "common.h"

class Sheet : public SheetInterface {
 public:
  ~Sheet();

  void SetCell(Position position, std::string text) override;
  CellInterface* GetCellInterface(Position position) override;
  const CellInterface* GetCellInterface(Position position) const override;
  Cell* GetCell(Position position);
  const Cell* GetCell(Position position) const;
  void ClearCell(Position position) override;
  Size GetPrintableSize() const override;
  void PrintValues(std::ostream& output) const override;
  void PrintTexts(std::ostream& output) const override;

 private:
  std::vector<std::vector<std::unique_ptr<Cell>>> spreadsheet_;
};