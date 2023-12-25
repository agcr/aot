﻿export var SynanDaemonUrl = 'http://localhost:17017?dummy=1';
export var SemanDaemonUrl = 'http://localhost:17018?dummy=1';

//export var SynanDaemonUrl = 'http://63.250.59.227:8080?dummy=1';
//export var SemanDaemonUrl = 'http://63.250.59.227:8081?dummy=1';



export const TableBorderStyle  = '1px solid black';

export function AddCell (tr, text) {
    var td = tr.insertCell()
    td.appendChild(document.createTextNode(text));
    td.style.border = TableBorderStyle;
    td.cellSpacing = "0";
    td.cellPadding = "10";
}

export function CreateTable() {
    var tbl  = document.createElement('table');
    tbl.style.width  = '500px';
    tbl.style.border = TableBorderStyle;
    tbl.style.borderCollapse = "collapse";
    return tbl;
}
                                               
