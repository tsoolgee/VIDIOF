# VideoFilter

DirectShow Source Filter עבור קבצי `.VIDEO` (MP4/MKV/TS ועוד עם 64 בייטים padding).

## בנייה

1. Push לGitHub
2. Actions → Build VideoFilter → הורד את `VideoFilter.ax`

## התקנה

פתח CMD כ-Administrator בתיקיית MPC-HC:

```cmd
regsvr32 VideoFilter.ax
```

## הסרה

```cmd
regsvr32 /u VideoFilter.ax
```

## הגדרה ב-MPC-HC

Options → Player → Formats → הוסף `VIDEO` לרשימה
