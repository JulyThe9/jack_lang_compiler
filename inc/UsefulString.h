#include <iostream>
#include <string>

struct UsefulString
{
private:
    int start;
    int end;
    int cur;
    bool eol;
    std::string str;
public:    
    UsefulString(const std::string &str) : str(str)
    {
        start = 0;
        end = str.size() - 1;
        cur = 0;
        eol = start > end;
    }

    void trimSpaces()
    {
        while (getChar() == ' ')
        {
            fwd();
        }
        updStart();
    }

    void skipSpaces()
    {
        while (getChar() == ' ')
        {
            if (!fwd())
                break;
        }
    }

    bool checkComment()
    {
        // no other operator starting with so, so it's a comment
        if (getChar() == '/')
        {
            return true;    
        }
        return false;
    }

    char getChar() const
    {
        return str[cur];
    }

    const std::string &getStr() const
    {
        return str;
    }

    const std::string getActual() const
    {
        return str.substr(start, end - start + 1);
    }

    bool islastchar()
    {
        if (!fwd()) return true;
        bwd();
        return false;
    }

    bool isEol()
    {
        return eol;
    }

    bool fwd()
    {   
        if (cur < end)
        {
            cur++;
            return true;
        }
        eol = true;
        return false;
    }
    bool bwd()
    {   
        if (cur > 0)
        {
            cur--;
            return true;
        }
        eol = false;
        return false;
    }
    void updStart() { start = cur; }
    // we now where we end at end+1 idx
    void updEnd() { end = cur - 1; }

    void updStart(int newStart) { start = newStart; }
    void updEnd(int newEnd) { end = newEnd; }
    void updCur(int newCur) { cur = newCur; }

    int getStart() const { return start; }
    int getEnd() const { return end; }
    int getCur() const { return cur; }

    void refreshCur() { cur = start; }

    void print() const
    {
        std::cout << str.substr(end - start + 1) << '\n';
    }
    bool operator< (const UsefulString &rhs) const
    {
        return str.substr(start, end - start + 1) < rhs.getStr().substr(rhs.getStart(), rhs.getEnd() - rhs.getStart() + 1);
    }

    bool operator==(const UsefulString &rhs) const 
    { 
        return (str.substr(start, end - start + 1).compare(rhs.getStr().substr(rhs.getStart(),
            rhs.getEnd() - rhs.getStart() + 1)) == 0);
    }
};