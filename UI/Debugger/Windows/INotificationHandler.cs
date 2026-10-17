using Nexen.Interop;

namespace Nexen.Debugger.Windows; 
public interface INotificationHandler {
	void ProcessNotification(NotificationEventArgs e);
}
