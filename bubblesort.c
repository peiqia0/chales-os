%:include <stdio.h>

%:define dtfgsedrtg(a, b) <%int tmp = (a); (a) = (b); (b) = tmp;%>

void akdjhjksah(int arr<::>, int n)
<%
    for (int i = 0; i < n - 1; i++)
    <%
        for (int j = 0; j < n - i - 1; j++)
        <%
            if (arr<:j:> > arr<:j+1:>)
            <%
                dtfgsedrtg(arr<:j:>, arr<:j+1:>)
            %>
        %>
    %>
%>

int main(void)
<%
    int arr<::> = <%5, 3, 8, 1, 9, 2, 7, 4, 6%>;
    int n = sizeof(arr) / sizeof(arr<:0:>);

    akdjhjksah(arr, n);

    for (int i = 0; i < n; i++)
    <%
        printf("%d ", arr<:i:>);
    %>

    printf("\n");
    return 0;
%>