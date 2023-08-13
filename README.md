# Spreadsheet

Spreadsheet project - simplified analogue of existing solutions: a Microsoft Excel spreadsheet or Google Sheets. The table cells can contain text or formulas. Formulas, as in existing solutions, can contain cell indexes.

## Build and launch

To build our application and create a Docker image, it will be enough to run the following command:

`docker build -t spreadsheet .`

To launch the application, use the command:

`docker run -it spreadsheet`
