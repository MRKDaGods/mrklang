using FastColoredTextBoxNS;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using Keys = System.Windows.Forms.Keys;
using Range = FastColoredTextBoxNS.Range;

namespace MRK;

public class CustomSyntaxHighlighter : SyntaxHighlighter
{
    // Text styles for syntax highlighting
    private readonly TextStyle _keywordStyle;
    private readonly TextStyle _typeStyle;
    private readonly TextStyle _stringStyle;
    private readonly TextStyle _commentStyle;
    private readonly TextStyle _numberStyle;
    private readonly TextStyle _operatorStyle;
    private readonly TextStyle _accessModStyle;
    private readonly TextStyle _blockStyle;
    private readonly TextStyle _blockContentStyle;

    // Auto-complete list
    private AutocompleteMenu? _autocompleteMenu;
    private readonly List<string> _keywords;
    private readonly List<string> _snippets;
    private readonly List<string> _types;
    private readonly List<string> _functions;

    public CustomSyntaxHighlighter(FastColoredTextBox textBox) : base(textBox)
    {
        // Initialize text styles with VS 2022 dark theme colors
        _keywordStyle = new TextStyle(new SolidBrush(Color.FromArgb(86, 156, 214)), null, FontStyle.Regular);   // blue
        _typeStyle = new TextStyle(new SolidBrush(Color.FromArgb(78, 201, 176)), null, FontStyle.Regular);      // teal
        _stringStyle = new TextStyle(new SolidBrush(Color.FromArgb(214, 157, 133)), null, FontStyle.Regular);   // orange-brown
        _commentStyle = new TextStyle(new SolidBrush(Color.FromArgb(87, 166, 74)), null, FontStyle.Regular);    // green
        _numberStyle = new TextStyle(new SolidBrush(Color.FromArgb(181, 206, 168)), null, FontStyle.Regular);   // light green
        _operatorStyle = new TextStyle(new SolidBrush(Color.FromArgb(215, 215, 215)), null, FontStyle.Regular); // light gray
        _accessModStyle = new TextStyle(new SolidBrush(Color.FromArgb(86, 156, 214)), null, FontStyle.Regular); // blue
        _blockStyle = new TextStyle(new SolidBrush(Color.FromArgb(206, 145, 120)), null, FontStyle.Regular);    // orange
        _blockContentStyle = new TextStyle(new SolidBrush(Color.FromArgb(170, 170, 170)), null, FontStyle.Regular);

        // Initialize auto-complete lists
        _keywords =
        [
            "func", "class", "struct", "enum", "interface", "var", "if", "else",
            "for", "foreach", "while", "return", "new", "delete", "in", "as",
            "params", "namespace", "__declspec", "use", "from", "global", "null"
        ];

        _types =
        [
            "int", "float", "double", "string", "bool", "char", "void", "object",
            "byte", "short", "long", "decimal"
        ];

        _functions = [];

        _snippets =
        [
            "if (|) {\n    \n}",
            "for (var<int> i = 0; i < |; i++) {\n    \n}",
            "foreach (var item in |) {\n    \n}",
            "while (|) {\n    \n}",
            "func | () {\n    \n}",
            "func | () -> returnType {\n    \n}",
            "class | {\n    \n}",
            "namespace | {\n    \n}",
            "cpp {\n    |\n}"
        ];

        // Set up text box event handlers
        textBox.TextChanged += TextBox_TextChanged;
        textBox.KeyDown += TextBox_KeyDown;
        textBox.Language = Language.Custom;

        // Configure auto-completion
        SetupAutoComplete(textBox);

        // Load existing AST data if available
        //LoadASTCompletionData();
    }

    private void SetupAutoComplete(FastColoredTextBox textBox)
    {
        // Create auto-complete menu
        _autocompleteMenu = new AutocompleteMenu(textBox)
        {
            MinFragmentLength = 2,
            AllowTabKey = true,
            AlwaysShowTooltip = true,
            AppearInterval = 500,
            ToolTipDuration = 5000,
            ForeColor = Color.FromArgb(220, 220, 220),
            BackColor = Color.FromArgb(30, 30, 30),
            SelectedColor = Color.FromArgb(51, 51, 51),
            SearchPattern = @"[\w\.:_\->]+"
        };

        // Populate autocomplete items
        var items = new List<AutocompleteItem>();

        // Add keywords
        foreach (var keyword in _keywords)
        {
            var item = new AutocompleteItem(keyword)
            {
                MenuText = keyword,
                ToolTipTitle = "Keyword",
                ToolTipText = $"Language keyword: {keyword}"
            };
            items.Add(item);
        }

        // Add types
        foreach (var type in _types)
        {
            var item = new AutocompleteItem(type)
            {
                MenuText = type,
                ToolTipTitle = "Type",
                ToolTipText = $"Built-in type: {type}"
            };
            items.Add(item);
        }

        // Add functions
        foreach (var function in _functions)
        {
            var item = new MethodAutocompleteItem(function)
            {
                MenuText = function,
                ToolTipTitle = "Function"
            };
            items.Add(item);
        }

        // Add code snippets
        foreach (var snippet in _snippets)
        {
            var item = new SnippetAutocompleteItem(snippet)
            {
                MenuText = snippet.Split('|')[0].TrimEnd(),
                ToolTipTitle = "Snippet",
                ToolTipText = snippet.Replace("|", "...")
            };
            items.Add(item);
        }

        // Set autocomplete source
        _autocompleteMenu.Items.SetAutocompleteItems(items);
    }

    private void TextBox_KeyDown(object? sender, System.Windows.Forms.KeyEventArgs e)
    {
        var textBox = sender as FastColoredTextBox;
        if (textBox == null) return;

        // Handle auto-indentation for opening braces
        if (e.KeyCode == Keys.Enter)
        {
            var line = textBox.Lines[textBox.Selection.Start.iLine];
            int indent = GetIndentLevel(line);
            bool hasOpenBrace = line.TrimEnd().EndsWith('{');

            if (hasOpenBrace)
            {
                // Insert newline with increased indentation with tabs
                textBox.InsertText("\n" + new string(' ', indent + 4));
                //textBox.Selection.Start = textBox.Selection.Start.AddLines(1);
                textBox.Selection.Start = new Place(indent + 4, textBox.Selection.Start.iLine);
                e.Handled = true;
            }
        }

        // Auto-close brackets and braces
        if (e.KeyCode == Keys.OemOpenBrackets)
        {
            if (e.Shift) // curly braces
            {
                textBox.InsertText("{}");
                textBox.Selection = new Range(
                    textBox,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine);

                e.Handled = true;
            }
            else // square brackets
            {
                textBox.InsertText("[]");
                textBox.Selection = new Range(
                    textBox,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine);

                e.Handled = true;
            }
        }
        else if (e.KeyCode == Keys.D9 && e.Shift) // parentheses
        {
            textBox.InsertText("()");
            textBox.Selection = new Range(
                    textBox,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine);

            e.Handled = true;
        }
        else if (e.KeyCode == Keys.OemQuotes && !e.Shift) // double quotes
        {
            textBox.InsertText("\"\"");
            textBox.Selection = new Range(
                    textBox,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine,
                    textBox.Selection.Start.iChar - 1,
                    textBox.Selection.Start.iLine);

            e.Handled = true;
        }
    }

    private int GetIndentLevel(string line)
    {
        int spaces = 0;
        foreach (char c in line)
        {
            if (c == ' ') spaces++;
            else if (c == '\t') spaces += 4;
            else break;
        }
        return spaces;
    }

    private void TextBox_TextChanged(object? sender, TextChangedEventArgs e)
    {
        var textBox = sender as FastColoredTextBox;

        e.ChangedRange.ClearStyle(StyleIndex.All);

        // Apply syntax highlighting styles

        // First, process comments to avoid highlighting code inside comments
        e.ChangedRange.SetStyle(_commentStyle, @"//.*$", RegexOptions.Multiline);
        e.ChangedRange.SetStyle(_commentStyle, @"/\*[\s\S]*?\*/", RegexOptions.Singleline);

        // Highlight string literals
        e.ChangedRange.SetStyle(_stringStyle, @"""(?:[^""\\]|\\.)*""");

        // Highlight numbers (integer, float, hex)
        e.ChangedRange.SetStyle(_numberStyle, @"\b(0x[0-9a-fA-F]+|\d+\.\d+|\d+)\b");

        // Highlight operators and punctuation
        e.ChangedRange.SetStyle(_operatorStyle, @"(\+\+|--|->|=>|::|\+=|-=|\*=|/=|==|!=|<=|>=|&&|\|\||!|\+|-|\*|/|%|=|<|>|\.|&|\||\^|~|\?|:|\(|\)|\{|\}|\[|\]|;|,)");

        // Highlight keywords
        e.ChangedRange.SetStyle(_keywordStyle, @"\b(func|class|struct|enum|interface|var|if|else|for|foreach|while|return|new|delete|in|as|params|namespace|__declspec|use|from|global|null)\b");

        // Highlight access modifiers
        e.ChangedRange.SetStyle(_accessModStyle, @"\b(public|protected|private|internal|static|abstract|sealed|virtual|override|const|readonly|extern|implicit|explicit|async)\b");

        // Highlight language blocks
        e.ChangedRange.SetStyle(_blockStyle, @"\b(csharp|cpp|dart|js)\b");

        // Highlight common types
        e.ChangedRange.SetStyle(_typeStyle, @"\b(int|float|double|string|bool|char|void|object|byte|short|long|decimal)(\[\])?(\*)?\b");

        // Highlight class names
        e.ChangedRange.SetStyle(_typeStyle, @"\b(class|struct|interface)\s+[a-zA-Z0-9_]*\b");

        // Highlight function names
        e.ChangedRange.SetStyle(_keywordStyle, @"\bfunc\s+[a-zA-Z_][a-zA-Z0-9_]*\b");

        // Highlight variable names
        e.ChangedRange.SetStyle(_typeStyle, @"\bvar\s*<\s*[a-zA-Z_][a-zA-Z0-9_:]*\s*>\s*[a-zA-Z_][a-zA-Z0-9_]*\b");

        // Highlight function return types
        e.ChangedRange.SetStyle(_typeStyle, @"\bfunc\s+[a-zA-Z_][a-zA-Z0-9_]*\(.*\)\s*(->\s*([a-zA-Z_][a-zA-Z0-9_:]*(\[\])?(\*)?))?");

        // Handle interpolated strings
        e.ChangedRange.SetStyle(_stringStyle, @"\$""(?:[^""\\]|\\.|\{[^}]*\})*""");

        // Handle language block literals - capture block markers first
        e.ChangedRange.SetStyle(_blockStyle, @"__(csharp|cpp|dart|js)\s*\{");
        e.ChangedRange.SetStyle(_blockStyle, @"\}");

        // Handle block content in language blocks
        var fullText = textBox!.Text;
        var blockMatches = Regex.Matches(fullText, @"__(csharp|cpp|dart|js)\s*\{([\s\S]*?)\}");
        foreach (Match match in blockMatches)
        {
            if (match.Groups.Count >= 3)
            {
                // Get the position of the content group
                var contentGroup = match.Groups[2];
                int startPos = contentGroup.Index;
                int endPos = startPos + contentGroup.Length;

                // Create a range for the content and apply the style
                var startPlace = textBox.PositionToPlace(startPos);
                var endPlace = textBox.PositionToPlace(endPos);
                var contentRange = new Range(textBox, startPlace, endPlace);

                // Check if this content intersects with the changed range
                if (contentRange.GetIntersectionWith(e.ChangedRange).Length > 0)
                {
                    contentRange.ClearStyle(StyleIndex.All);
                    contentRange.SetStyle(_blockContentStyle);
                }
            }
        }

        // Update auto-completion context based on current content
        UpdateAutoCompleteContext(e.ChangedRange);
    }

    private void UpdateAutoCompleteContext(Range range)
    {
        // Refresh auto-complete suggestions from AST if possible
        //RefreshASTCompletionData();
    }

    #region AST and C++ Interop

    [DllImport("mrklang")]
    private static extern IntPtr GetASTSymbols(string code);

    [DllImport("mrklang")]
    private static extern void FreeASTSymbols(IntPtr symbols);

    [StructLayout(LayoutKind.Sequential)]
    private struct ASTSymbol
    {
        public IntPtr Name;
        public int Type;
        public IntPtr Signature;
    }

    private void LoadASTCompletionData()
    {
        try
        {
            // This function would be called when loading a file or refreshing completion data
            // For now, we'll populate with some reasonable defaults from the AST data we've seen
            // In a real implementation, this would call into the C++ AST

            _functions.Add("func main() -> void");
            _functions.Add("func getSignature() -> string");
            _functions.Add("func getTypeName() -> string");
            _functions.Add("func toString() -> string");
            _functions.Add("func accept(visitor) -> void");

            // Update auto-complete menu
            // SetupAutoComplete(TextBox);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error loading AST data: {ex.Message}");
        }
    }

    #endregion
}