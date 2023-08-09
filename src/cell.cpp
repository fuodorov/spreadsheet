#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <set>
#include <stack>

Cell::Cell(Sheet& sheet) : impl_(std::make_unique<EmptyImpl>()),
                           sheet_(sheet) {}
Cell::~Cell() = default;

void Cell::Set(std::string text, Position pos, Sheet* sheet) { // тут pos - это позиция ячейки для которой устанавливаем значение а text - это устанавливаемое значение, которое может быть формулой
    std::unique_ptr<Impl> impl;
    std::forward_list<Position> set_formula_cells;

    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    } else if (text.size() >= 2 && text[0] == FORMULA_SIGN) {
        impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
        std::vector<Position> pos_cell_in_formula = impl->GetReferencedCells();
        for (Position pos_cell_in_formula : pos_cell_in_formula ){

            if (pos_cell_in_formula.IsValid() && !sheet->GetCell(pos_cell_in_formula)) {
                sheet->SetCell(pos_cell_in_formula, "");
            }
        }

    } else {
        impl= std::make_unique<TextImpl>(std::move(text));
    }

    if (CheckCircularDependencies(*impl,  pos)) {

        throw CircularDependencyException("circular dependency detected");
    }

    // Обновление зависимостей
    auto second_st = this->pos_.ToString();

    impl_ = std::move(impl);

    for (Cell* used : using_cells_) {
        auto third_st = used->pos_.ToString();
        used->calculated_cells_.erase(this);
    }

    using_cells_.clear();

    auto iterim_st = impl_->GetText();
    for (const auto& pos : impl_->GetReferencedCells()) {
        auto four_st = pos_.ToString();
        Cell* used = sheet_.GetConcreteCell(pos);
        if (!used ) {
            sheet_.SetCell(pos, "");
            used  = sheet_.GetConcreteCell(pos);
        }
        auto five_st = used->pos_.ToString();
        //auto six_st = used->GetValue();
        using_cells_.insert(used );
        used ->calculated_cells_.insert(this);
    }

    // Инвалидация кеша
    ResetCache(true);
}



bool Cell::HasCircularDependency( Cell* cell,  std::unordered_set<Cell*>& visitedPos, const Position pos_const) {

    for (auto dependentPos : cell->GetReferencedCells()) {
        Cell* ref_cell = sheet_.GetConcreteCell(dependentPos);
        if (pos_const == dependentPos){return  true;}

        if (visitedPos.find(ref_cell) == visitedPos.end() ) {
            visitedPos.insert(ref_cell);
            if (HasCircularDependency(ref_cell, visitedPos,  pos_const))
                return true;
        }
    }

    return false;
}

bool Cell::CheckCircularDependencies( const Impl& new_impl, Position pos) {
    const Position pos_const = pos;
    const auto& cells = new_impl.GetReferencedCells();
    std::unordered_set<Cell*> visitedPos;
    for ( const auto& position : cells) {
        if (position == pos) {
            return true; /*CircularDependencyException("circular dependency detected");*/
        }
        Cell* ref_cell = sheet_.GetConcreteCell(position);
        //std::set<Cell*> visitedPos;
        visitedPos.insert(ref_cell);
        if (HasCircularDependency(ref_cell, visitedPos , pos_const)){
            return true;
        }
    }
    return false;
}

void Cell::Clear() {
    Set("", pos_, &sheet_);
}

Cell::Value Cell::GetValue() const {return impl_->GetValue();}
std::string Cell::GetText() const {return impl_->GetText();}
std::vector<Position> Cell::GetReferencedCells() const {return impl_->GetReferencedCells();}
bool Cell::IsReferenced() const {return !calculated_cells_.empty();}

void Cell::ResetCache(bool force /*= false*/) {
    if (impl_->HasCache() || force) {
        impl_->ResetCache();

        for (Cell* dependent : calculated_cells_) {
            dependent->ResetCache();
        }
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {return {};}
bool Cell::Impl::HasCache() {return true;}
void Cell::Impl::ResetCache() {}

Cell::Value Cell::EmptyImpl::GetValue() const {return "";}
std::string Cell::EmptyImpl::GetText() const {return "";}

Cell::TextImpl::TextImpl(std::string text) : text_(std::move(text)) {}

Cell::Value Cell::TextImpl::GetValue() const {

    if (text_.empty()) {
        throw FormulaException("it is empty impl, not text");

    } else if (text_.at(0) == ESCAPE_SIGN) {
        return text_.substr(1);

    } else {
        return text_;
    }
}

std::string Cell::TextImpl::GetText() const {return text_;}

Cell::FormulaImpl::FormulaImpl(std::string text, SheetInterface& sheet) : formula_ptr_(ParseFormula(text.substr(1)))
        , sheet_(sheet) {}

Cell::Value Cell::FormulaImpl::GetValue() const {

    if (!cache_) {cache_ = formula_ptr_->Evaluate(sheet_);}
    return std::visit([](auto& helper){return Value(helper);}, *cache_);
}

std::string Cell::FormulaImpl::GetText() const {return FORMULA_SIGN + formula_ptr_->GetExpression();}
std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {return formula_ptr_->GetReferencedCells();}
bool Cell::FormulaImpl::HasCache() {return cache_.has_value();}
void Cell::FormulaImpl::ResetCache() {cache_.reset();}