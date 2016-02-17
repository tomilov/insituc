#pragma once

#include <insituc/ast/io.hpp>
#include <insituc/meta/assembler.hpp>

#include <ostream>

namespace insituc
{
namespace meta
{

inline
std::ostream &
operator << (std::ostream & _out, mnemocode const _mnemocode)
{
    return _out << c_str(_mnemocode);
}

inline
std::ostream &
operator << (std::ostream & _out, st_type const & _st)
{
    if (_st == 0) {
        return _out << "st";
    } else {
        return _out << "st(" << _st << ')';
    }
}

inline
std::ostream &
operator << (std::ostream & _out, memory_layout const _memory_layout)
{
    return _out << c_str(_memory_layout);
}

inline
std::ostream &
operator << (std::ostream & _out, instruction_nullary const & _instruction)
{
    return _out << _instruction.mnemocode_;
}

inline
std::ostream &
operator << (std::ostream & _out, instruction_unary const & _instruction)
{
    return _out << _instruction.mnemocode_
                << ' '
                << _instruction.operand_;
}

inline
std::ostream &
operator << (std::ostream & _out, instruction_binary const & _instruction)
{
    return _out << _instruction.mnemocode_
                << ' '
                << _instruction.destination_
                << ", "
                << _instruction.source_;
}

inline
std::ostream &
operator << (std::ostream & _out, instruction_auxiliary const & _instruction)
{
    return _out << _instruction.mnemocode_
                << ' '
                << _instruction.memory_layout_
                << " #"
                << _instruction.offset_;
}

inline
std::ostream &
operator << (std::ostream & _out, function const & _function)
{
    _out << "function name: " << _function.symbol_ << '\n';
    size_type const arity_ = _function.arity();
    assert(!(arity_ < _function.input_));
    _out << " arity: " << arity_ << '\n';
    symbols_type const & arguments_ = _function.arguments_;
    if (0 < arity_) {
        size_type const stack_input_ = arity_ - _function.input_;
        _out << " stack input: " << stack_input_ << '\n';
        if (0 < stack_input_) {
            _out << " stack input arguments: ";
            for (size_type i = 0; i < stack_input_ - 1; ++i) {
                _out << arguments_[i] << ", ";
            }
            _out << arguments_[stack_input_ - 1] << '\n';
        }
        if (0 < _function.input_) {
            _out << " floating-point registers input arguments: ";
            for (size_type i = stack_input_; i < arity_ - 1; ++i) {
                _out << arguments_[i] << ", ";
            }
            _out << arguments_[arity_ - 1] << '\n';
        }
    }
    _out << " floating-point registers output: " << _function.output_ << '\n';
    _out << " floating-point registers clobbered: " << _function.clobbered_ << '\n';
    _out << " frame clobbered: " << _function.frame_clobbered_ << '\n';
    _out << " stack climbing: " << _function.climbing_ << '\n';
    _out << " code:\n";
    for (instruction const & instruction_ : _function.code_) {
        _out << '\t' << instruction_ << '\n';
    }
    return _out;
}

struct call_graph_printer
{

    call_graph_printer(std::ostream & _out,
                       assembler const & _assembler)
        : out_(_out)
        , assembler_(_assembler)
    {
        size_type const size_ = assembler_.get_export_table().size();
        auto nvbeg = std::begin(nonvisited_);
        for (size_type i = 0; i < size_; ++i) {
            nvbeg = nonvisited_.insert(nvbeg, i);
        }
    }

    void
    operator () ()
    {
        while (!nonvisited_.empty()) {
            operator () (*nonvisited_.cbegin());
            out_ << '\n';
        }
    }

private :

    std::ostream & out_;
    assembler const & assembler_;
    size_type indent_ = 1;
    std::set< size_type, std::greater< size_type > > nonvisited_;

    void
    operator () (size_type const _caller)
    {
        nonvisited_.erase(_caller);
        for (size_type i = 0; i < indent_; ++i) {
            out_.put('\t');
        }
        function const & function_ = assembler_.get_function(_caller);
        {
            out_ << function_.symbol_ << '(';
            if (!function_.arguments_.empty()) {
                for (symbol_type const & argument_ : head(function_.arguments_)) {
                    out_ << argument_ << ", ";
                }
                out_ << function_.arguments_.back();
            }
            out_ << ") [" << function_.output_ << "]\n";
        }
        std::unordered_set< size_type > const & callies_ = assembler_.get_function(_caller).callies_;
        if (!callies_.empty()) { // not leaf?
            assert(indent_ < std::numeric_limits< size_type >::max());
            ++indent_;
            for (size_type const caller_ : callies_) {
                operator () (caller_);
            }
            assert(0 < indent_);
            --indent_;
        }
    }

};

inline
std::ostream &
operator << (std::ostream & _out, assembler const & _assembler)
{
    if (_assembler.empty()) {
        return _out << "assembler instance is empty\n";
    }
    size_type const size_ = _assembler.get_export_table().size();
    _out << "instance with " << size_ << " functions\n"
         << "maximum stack depth used: " << _assembler.get_stack_size() << '\n';
    {
        _out << "function call graph (tree):\n";
        call_graph_printer call_graph_printer_(_out, _assembler);
        call_graph_printer_(); // print call tree
    }
    size_type const heap_size_ = _assembler.get_heap_size();
    _out << "heap size: " << heap_size_ << '\n';
    if (0 < heap_size_) {
        symbol_mapping_type const & heap_symbols_ = _assembler.get_heap_symbols();
        size_type const symbols_count_ = heap_symbols_.size();
        _out << "heap consists " << symbols_count_ << " global variables and " << (heap_size_ - symbols_count_) << " literals\n";
        if (!heap_symbols_.empty()) {
            _out << "global variables (symbol at offset eq value):\n";
            for (auto const & heap_symbol_ : heap_symbols_) {
                _out << ' ' << heap_symbol_.first << " @ " << heap_symbol_.second << " = " << _assembler.get_heap_element(heap_symbol_.second) << '\n';
            }
        }
    }
    _out << "functions info:\n";
    for (size_type i = 0; i < size_; ++i) {
        _out << _assembler.get_function(i) << '\n';
    }
    return _out;
}

}
}
