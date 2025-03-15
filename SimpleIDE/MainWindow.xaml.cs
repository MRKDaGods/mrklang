using FastColoredTextBoxNS;
using System.Windows;
using System.Windows.Controls.Primitives;
using System.Drawing;
using System.Collections.Generic;
using WF = System.Windows.Forms;
using System.IO;
using System.Windows.Controls;
using System.Linq;
using System.ComponentModel;

namespace MRK;

/// <summary>
/// Interaction logic for MainWindow.xaml
/// </summary>
public partial class MainWindow : Window
{
    class SolutionFile
    {
        public required string FileName { get; set; }
        public required string Path { get; set; }
    }

    class Editor : INotifyPropertyChanged
    {
        private bool _isDirty;

        public event PropertyChangedEventHandler? PropertyChanged;

        public SolutionFile SolutionFile { get; init; }
        public FastColoredTextBox TextBox { get; init; }
        public bool IsCurrent { get; set; }
        public bool IsDirty
        {
            get => _isDirty;
            set
            {
                if (_isDirty != value)
                {
                    _isDirty = value;
                    NotifyPropertyChanged(nameof(DisplayName));
                }
            }
        }

        public string DisplayName => SolutionFile.FileName + (IsDirty ? "*" : "");

        public Border? Control { get; set; }

        public Editor(SolutionFile file)
        {
            SolutionFile = file;
            TextBox = new FastColoredTextBox()
            {
                BackColor = Color.FromArgb(32, 32, 32),
                ForeColor = Color.FromArgb(241, 241, 241),

                ServiceLinesColor = Color.FromArgb(32, 32, 32),

                Font = new Font("Consolas", 10f, GraphicsUnit.Point),

                CaretColor = Color.White,
                Paddings = new WF.Padding(5),

                ShowLineNumbers = true,
                IndentBackColor = Color.FromArgb(32, 32, 32),
                CurrentLineColor = Color.FromArgb(200, 200, 200),
                LineNumberColor = Color.FromArgb(100, 100, 100),
                ReservedCountOfLineNumberChars = 2,

                TextAreaBorder = TextAreaBorderType.None,

                SelectionColor = Color.FromArgb(0x11, 0x3d, 0x6f),

                Language = FastColoredTextBoxNS.Language.Custom
            };

            TextBox.SyntaxHighlighter = new CustomSyntaxHighlighter(TextBox);

            TextBox.TextChanged += (_, x) =>
            {
                IsDirty = true;
            };

            TextBox.SelectionChanged += (_, _) => UpdateInfoText();
            TextBox.ZoomChanged += (_, _) => UpdateInfoText();

            // Handle save
            TextBox.KeyDown += (_, x) =>
            {
                if (x.Control && x.KeyCode == WF.Keys.S)
                {
                    TextBox.SaveToFile(file.Path, System.Text.Encoding.UTF8);
                    IsDirty = false;
                }
            };

            TextBox.OpenFile(file.Path);
            IsDirty = false;
        }

        private void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public void Save()
        {
            TextBox.SaveToFile(SolutionFile.Path, System.Text.Encoding.UTF8);
            IsDirty = false;
        }

        public void UpdateInfoText()
        {
            if (IsCurrent)
            {
                Instance?.SetInfoText(TextBox.Selection.Start.iLine + 1, TextBox.Selection.Start.iChar + 1, TextBox.Zoom, "UTF-8");
            }
        }
    }

    // Too lazy to split em
    class SolutionExplorer
    {
        private readonly Dictionary<SolutionFile, TreeViewItem> _treeItems = [];
        private bool _ignoreSelectionChange;

        private MainWindow Owner { get; init; }
        public List<SolutionFile> SolutionFiles { get; init; }

        public SolutionExplorer(MainWindow owner)
        {
            Owner = owner;
            owner.openFolderButton.Click += (_, _) => OpenFolder();
            owner.solutionControlsPanel.Children.OfType<Button>().ToList().ForEach(x => x.Click += OnControlPanelButtonClick);

            SolutionFiles = [];

            // by def no folder is opened
            Refresh();
        }

        public void SetVisible(bool isVisible)
        {
            Owner.solutionExplorerRootContainer.Visibility = isVisible ? Visibility.Visible : Visibility.Collapsed;
        }

        public void OpenFolder()
        {
            if (!string.IsNullOrEmpty(Owner._currentlyOpenedFolder))
            {
                var result = MessageBox.Show("Are you sure you want to open a new folder? All unsaved changes will be lost.", "Open Folder", MessageBoxButton.YesNo, MessageBoxImage.Warning);

                if (result != MessageBoxResult.Yes) return;
            }

            Owner.Reset();

            var dialog = new WF.FolderBrowserDialog();
            if (dialog.ShowDialog() == WF.DialogResult.OK)
            {
                Owner._currentlyOpenedFolder = dialog.SelectedPath;
                Refresh();
            }
        }

        private void Refresh()
        {
            var hasFolder = !string.IsNullOrEmpty(Owner._currentlyOpenedFolder)
                && Directory.Exists(Owner._currentlyOpenedFolder);

            Owner.noSolutionContentPanel.Visibility = hasFolder ? Visibility.Collapsed : Visibility.Visible;
            Owner.solutionContentPanel.Visibility = hasFolder ? Visibility.Visible : Visibility.Collapsed;

            if (hasFolder)
            {

                _treeItems.Clear();

                // Clear all items
                var view = Owner.solutionExplorerView;
                view.Items.Clear();

                // Root node
                var root = new TreeViewItem
                {
                    Header = Owner._currentlyOpenedFolder,
                    IsExpanded = true,
                    ContextMenu = CreateContextMenu(new SolutionFile { FileName = Owner._currentlyOpenedFolder, Path = Owner._currentlyOpenedFolder }, true),
                    Tag = Owner._currentlyOpenedFolder
                };
                view.Items.Add(root);

                // Recursively add all files and folders
                AddFilesAndFolders(Owner._currentlyOpenedFolder, root);

                view.SelectedItemChanged += (_, _) =>
                {
                    if (_ignoreSelectionChange) return;

                    if (view.SelectedItem is TreeViewItem item)
                    {
                        Owner.SetOpenedFile(item.Tag as SolutionFile);
                    }
                };
            }
        }

        private void AddFilesAndFolders(string path, TreeViewItem parent)
        {
            foreach (var file in Directory.GetFiles(path))
            {
                var solFile = new SolutionFile
                {
                    FileName = Path.GetFileName(file),
                    Path = file,
                };

                var item = new TreeViewItem
                {
                    Header = Path.GetFileName(file),
                    Tag = solFile,
                    ContextMenu = CreateContextMenu(solFile)
                };
                parent.Items.Add(item);

                SolutionFiles.Add(solFile);

                _treeItems[solFile] = item;
            }

            foreach (var dir in Directory.GetDirectories(path))
            {
                var item = new TreeViewItem
                {
                    Header = Path.GetFileName(dir),
                    IsExpanded = true,
                    ContextMenu = CreateContextMenu(new SolutionFile { FileName = Path.GetFileName(dir), Path = dir }, true),
                    Tag = dir
                };
                parent.Items.Add(item);
                AddFilesAndFolders(dir, item);
            }
        }

        private ContextMenu CreateContextMenu(SolutionFile file, bool isDir = false)
        {
            var contextMenu = new ContextMenu();

            var deleteMenuItem = new MenuItem { Header = "Delete" };
            deleteMenuItem.Click += (s, e) => DeleteFile(file);
            contextMenu.Items.Add(deleteMenuItem);

            if (isDir)
            {
                var newFileMenuItem = new MenuItem { Header = "New File" };
                newFileMenuItem.Click += (s, e) => CreateNewFile(file.Path);
                contextMenu.Items.Add(newFileMenuItem);

                var newFolderMenuItem = new MenuItem { Header = "New Folder" };
                newFolderMenuItem.Click += (s, e) => CreateNewFolder(file.Path);
                contextMenu.Items.Add(newFolderMenuItem);
            }

            var renameMenuItem = new MenuItem { Header = "Rename" };
            renameMenuItem.Click += (s, e) => RenameFile(file);
            contextMenu.Items.Add(renameMenuItem);

            return contextMenu;
        }

        private void RenameFile(SolutionFile file)
        {
            var inputDialog = new InputDialog("Rename File", "Enter new name:", file.FileName);
            if (inputDialog.ShowDialog() ?? false)
            {
                var newFileName = inputDialog.ResponseText;
                var newPath = Path.Combine(Path.GetDirectoryName(file.Path)!, newFileName);

                if (File.Exists(file.Path))
                {
                    File.Move(file.Path, newPath);
                }
                else if (Directory.Exists(file.Path))
                {
                    Directory.Move(file.Path, newPath);
                }

                file.FileName = newFileName;
                file.Path = newPath;
                Refresh();
            }
        }

        private void DeleteFile(SolutionFile file)
        {
            if (MessageBox.Show($"Are you sure you want to delete {file.FileName}?", "Delete File", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                if (File.Exists(file.Path))
                {
                    File.Delete(file.Path);
                }
                else if (Directory.Exists(file.Path))
                {
                    Directory.Delete(file.Path, true);
                }
                Refresh();
            }
        }

        private void CreateNewFile(string path)
        {
            var inputDialog = new InputDialog("New File", "Enter file name:", "NewFile.mrk");
            if (inputDialog.ShowDialog() ?? false)
            {
                var newFileName = inputDialog.ResponseText;
                var newFilePath = Path.Combine(path, newFileName);
                File.Create(newFilePath).Close();
                Refresh();
            }
        }

        private void CreateNewFolder(string path)
        {
            var inputDialog = new InputDialog("New Folder", "Enter folder name:", "NewFolder");
            if (inputDialog.ShowDialog() ?? false)
            {
                var newFolderName = inputDialog.ResponseText;
                var newFolderPath = Path.Combine(path, newFolderName);
                Directory.CreateDirectory(newFolderPath);
                Refresh();
            }
        }

        public void SelectFile(SolutionFile file)
        {
            if (_treeItems.TryGetValue(file, out var item))
            {
                _ignoreSelectionChange = true;
                item.IsSelected = true;
                _ignoreSelectionChange = false;
            }
        }

        private void OnControlPanelButtonClick(object sender, RoutedEventArgs e)
        {
            if (sender is Button button)
            {
                switch (button.Tag)
                {
                    case "refresh":
                        Refresh();
                        break;

                    case "collapse":
                        // collapse tree view
                        Owner.solutionExplorerView.Items.Cast<TreeViewItem>().ToList().ForEach(x => x.IsExpanded = false);
                        break;

                    case "openFolder":
                        OpenFolder();
                        break;
                }
            }
        }
    }

    private readonly SolutionExplorer _solutionExplorer;
    private readonly Dictionary<SolutionFile, Editor> _openedEditors = [];

    private string _currentlyOpenedFolder = "";
    private Editor? _currentEditor;

    private static MainWindow? Instance { get; set; }

    public MainWindow()
    {
        Instance = this;

        InitializeComponent();

        _solutionExplorer = new SolutionExplorer(this);
        _solutionExplorer.SetVisible(true);

        // Set editors src
        openedFilesItemsControl.ItemsSource = _openedEditors.Values;

        SetOpenedFile(null);
    }

    protected override void OnClosing(CancelEventArgs e)
    {
        // Check if any editor is dirty
        if (_openedEditors.Values.Any(x => x.IsDirty))
        {
            // Display confirmation dialog
            var result = MessageBox.Show(
                "Save changes before closing?",
                "Save changes",
                MessageBoxButton.YesNoCancel,
                MessageBoxImage.Question
            );

            if (result == MessageBoxResult.Cancel)
            {
                e.Cancel = true;
                return;
            }

            if (result == MessageBoxResult.Yes)
            {
                foreach (var editor in _openedEditors.Values)
                {
                    if (editor.IsDirty)
                    {
                        editor.Save();
                    }
                }
            }
        }

        base.OnClosing(e);
    }

    private void OnSolutionExplorerToggleClick(object sender, RoutedEventArgs e)
    {
        if (sender is ToggleButton toggleButton)
        {
            _solutionExplorer.SetVisible(toggleButton.IsChecked ?? false);
        }
    }

    private void SetOpenedFile(SolutionFile? file)
    {
        if (_currentEditor != null && _currentEditor.SolutionFile != file)
        {
            _currentEditor.IsCurrent = false;
        }

        if (file == null)
        {
            Title = "mrklang";

            fctbHost.Visibility = Visibility.Collapsed;
            fctbHost.Child = null;

            _currentEditor = null;

            statusBar.Visibility = Visibility.Collapsed;
            return;
        }

        Title = $"mrklang - {file.FileName}";

        if (!_openedEditors.TryGetValue(file, out var editor))
        {
            editor = new Editor(file);
            _openedEditors[file] = editor;
        }

        _currentEditor = editor;
        _currentEditor.IsCurrent = true;

        fctbHost.Visibility = Visibility.Visible;
        fctbHost.Child = editor.TextBox;

        _solutionExplorer.SelectFile(file);

        statusBar.Visibility = Visibility.Visible;
        editor.UpdateInfoText();

        RefreshOpenedFilesTab();
    }

    private void OnFileTabClick(object sender, RoutedEventArgs e)
    {
        if (sender is Border b && b.Tag is Editor editor)
        {
            SetOpenedFile(editor.SolutionFile);
        }
    }

    private void OnFileTabCloseClick(object sender, RoutedEventArgs e)
    {
        if (sender is Button button && button.Tag is Editor editor)
        {
            if (editor.IsDirty)
            {
                // Display confirmation dialog
                var result = MessageBox.Show(
                    $"Save changes to {editor.SolutionFile.FileName}?",
                    "Save changes",
                    MessageBoxButton.YesNoCancel,
                    MessageBoxImage.Question
                );

                if (result == MessageBoxResult.Cancel) return;

                if (result == MessageBoxResult.Yes)
                {
                    // Save file
                    editor.Save();
                }
            }

            if (_currentEditor == editor)
            {
                var ls = _openedEditors.Values.ToList();
                var idx = ls.IndexOf(editor);
                var nextIdx = idx + 1;
                if (nextIdx >= _openedEditors.Count) nextIdx = idx - 1;

                SetOpenedFile(nextIdx >= 0 ? ls[nextIdx].SolutionFile : null);
            }

            _openedEditors.Remove(editor.SolutionFile);

            RefreshOpenedFilesTab();
        }
    }

    private void RefreshOpenedFilesTab()
    {
        // Force rebuild
        // HACK
        openedFilesItemsControl.ItemsSource = _openedEditors.Values.ToList();
        openedFilesItemsControl.Items.Refresh();
    }

    private void OnFileTabLoad(object sender, RoutedEventArgs e)
    {
        // Assign tab control
        if (sender is Border b && b.Tag is Editor editor)
        {
            editor.Control = b;
        }
    }

    private void Reset()
    {
        _openedEditors.Clear();
        SetOpenedFile(null);
    }

    private void SetInfoText(int line, int ch, int zoom, string encoding)
    {
        lnChLabel.Text = $"Ln: {line}   Ch: {ch}";
        zoomLabel.Text = $"{zoom}%";
        encodingLabel.Text = encoding;
    }
}