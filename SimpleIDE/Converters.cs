using System;
using System.Windows.Data;

namespace MRK;

public class BoolToAsteriskConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    {
        return (bool)value ? "*" : "";
    }
    public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
    {
        return (string)value == "*";
    }
}