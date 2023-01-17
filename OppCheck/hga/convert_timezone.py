from datetime import timezone
from datetime import datetime
dt = datetime.fromisoformat("2023-01-15 12:30")
timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
print(int(timestamp))