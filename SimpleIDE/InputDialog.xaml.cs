using System.Windows;

namespace MRK;

public partial class InputDialog : Window
{
    public string ResponseText { get; private set; } = string.Empty;

    public InputDialog(string title, string prompt, string defaultResponse = "")
    {
        InitializeComponent();
        Title = title;
        PromptText.Text = prompt;
        responseTextBox.Text = defaultResponse;
    }

    private void OnOkClick(object sender, RoutedEventArgs e)
    {
        ResponseText = responseTextBox.Text;
        DialogResult = true;
    }

    private void OnCancelClick(object sender, RoutedEventArgs e)
    {
        DialogResult = false;
    }
}
